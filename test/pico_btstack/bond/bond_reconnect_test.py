#!/usr/bin/env python3
"""
Regression test: a bond established with a Pico must survive a disconnect.

Drives a real Pico running real firmware, using Bumble as the peer:

    phase 1  connect, bond, disconnect
    phase 2  reconnect and re-establish security from the stored keys

No reboot is needed: BTstack frees the
hci_connection_t on disconnect and hci_connection_init() resets link_key_type to
INVALID_LINK_KEY, so the reconnect must be answered from the flash-backed TLV
database rather than the RAM cache.

Modes (--mode):

    ctkd     BR/EDR SSP bond, then SMP over BR/EDR to trigger Cross-Transport
             Key Derivation.
    classic  BR/EDR SSP bond only, no CTKD.
    le       LE SMP bond, then re-encrypt using the stored LTK.

Run all three, see test.sh

DUT firmware: pico_btstack_bond_test, built from bond_dut.c in this directory.
That is BTstack's spp_and_gatt_counter with bonding requested in
sm_set_authentication_requirements(), giving SPP over BR/EDR and an advertising
GATT server over LE, so one image serves all three modes. It advertises as
"PicoBondTest", which --name matches, so no per-board address is needed.

Usage:
    ./test.sh                                     # all modes, DUT found by name
    ./test.sh --flash                             # locates the built image too
    ./bond_reconnect_test.py --name PicoBondTest --mode le
    ./bond_reconnect_test.py --addr 2C:CF:67:BE:08:05 --mode ctkd -v

Exits 0 on pass, 1 on failure, 2 on setup/harness error.
"""

import argparse
import asyncio
import logging
import subprocess
import sys

from bumble.core import AdvertisingData, PhysicalTransport
from bumble.device import Device
from bumble.hci import Address, OwnAddressType
from bumble.keys import MemoryKeyStore
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.transport import open_transport

LOG = logging.getLogger("bond-reconnect")

# Give each phase a bound so a hung stack fails the test rather than the harness.
PHASE_TIMEOUT = 30.0
# Let the Pico settle between the disconnect and the reconnect.
SETTLE = 2.0
# Allow SMP key distribution to drain before tearing the link down.
KEY_DISTRIBUTION_SETTLE = 2.0
# How long to scan for the DUT when --name is used.
DISCOVERY_TIMEOUT = 20.0


def flash_pico(uf2: str) -> None:
    """Erase and reflash, guaranteeing a clean TLV bank (and so a clean bond)."""
    LOG.info("erasing flash")
    subprocess.run(["picotool", "erase"], check=True)
    LOG.info("loading %s", uf2)
    subprocess.run(["picotool", "load", "-x", uf2], check=True)


def make_device(hci_transport, keystore) -> Device:
    device = Device.with_hci(
        "bond-reconnect-tester",
        Address("F0:F1:F2:F3:F4:F5"),
        hci_transport.source,
        hci_transport.sink,
    )
    # Bumble is LE-only unless classic is turned on explicitly.
    device.classic_enabled = True
    device.keystore = keystore
    # Just Works: no MITM, no user interaction.
    device.pairing_config_factory = lambda _connection: PairingConfig(
        sc=True,
        mitm=False,
        bonding=True,
        delegate=PairingDelegate(PairingDelegate.NO_OUTPUT_NO_INPUT),
    )
    return device


# Which physical transport each mode bonds over.
MODE_TRANSPORT = {
    "ctkd": PhysicalTransport.BR_EDR,
    "classic": PhysicalTransport.BR_EDR,
    "le": PhysicalTransport.LE,
}


def connect_kwargs(mode: str) -> dict:
    """LE must connect using our PUBLIC address.

    Bumble distributes its identity as PUBLIC_DEVICE during SMP, but
    Device.connect() defaults own_address_type to RANDOM. The DUT would then
    store the bond against our public identity and, on reconnect, see a static
    random address that is not an RPA and cannot be resolved to it -- so it
    reports PIN_OR_KEY_MISSING and the test fails.
    BR/EDR always uses the public address, so this only affects LE.
    """
    kwargs = {"transport": MODE_TRANSPORT[mode]}
    if mode == "le":
        kwargs["own_address_type"] = OwnAddressType.PUBLIC
    return kwargs


async def discover_by_name(device: Device, name: str, timeout: float) -> Address:
    """Find the DUT by scanning for its advertised local name.

    A Pico advertises over LE using its public BD_ADDR, which is also its
    BR/EDR address, so an address found this way works for every mode.
    """
    loop = asyncio.get_running_loop()
    found: asyncio.Future = loop.create_future()

    def on_advertisement(advertisement) -> None:
        if found.done():
            return
        local_name = advertisement.data.get(AdvertisingData.COMPLETE_LOCAL_NAME)
        if local_name is None:
            local_name = advertisement.data.get(AdvertisingData.SHORTENED_LOCAL_NAME)
        if isinstance(local_name, (bytes, bytearray)):
            local_name = local_name.decode("utf-8", "replace")
        if local_name and name.lower() in local_name.lower():
            LOG.info("found '%s' at %s", local_name, advertisement.address)
            found.set_result(advertisement.address)

    device.on("advertisement", on_advertisement)
    await device.start_scanning(active=True)
    try:
        address = await asyncio.wait_for(found, timeout)
    finally:
        await device.stop_scanning()
        device.remove_listener("advertisement", on_advertisement)

    # Re-tag as a public address: BR/EDR connects need PUBLIC_DEVICE_ADDRESS.
    return Address(str(address).split("/")[0], Address.PUBLIC_DEVICE_ADDRESS)


async def bond(device: Device, addr: Address, mode: str) -> None:
    """Phase 1: establish a bond that must survive a disconnect."""
    connection = await device.connect(addr, **connect_kwargs(mode))
    try:
        if mode == "le":
            # LE SMP. Requires the DUT to request bonding, or no keys persist.
            LOG.info("connected, pairing over LE")
            await connection.pair()
        else:
            LOG.info("connected, authenticating")
            await connection.authenticate()
            LOG.info("authenticated, encrypting")
            await connection.encrypt()
            if mode == "ctkd":
                # SMP over BR/EDR (SMP_BR_CID). btstack answers this from
                # SM_BR_EDR_RESPONDER_PAIRING_REQUEST_RECEIVED
                LOG.info("encrypted, pairing over BR/EDR (CTKD)")
                await connection.pair()
            else:
                # 'classic' deliberately stops here: SSP bonding only, no CTKD.
                LOG.info("encrypted, skipping CTKD (classic mode)")
        LOG.info("bonded")
        # pair() resolves when *our* side of pairing is done, not when both ends
        # have finished distributing keys. Disconnecting immediately cuts the DUT
        # off mid key-distribution, so sm_key_distribution_handle_all_received()
        # never fires and it stores nothing -- the reconnect then fails with
        # PIN_OR_KEY_MISSING for a harness reason. Let distribution drain first.
        await asyncio.sleep(KEY_DISTRIBUTION_SETTLE)
    finally:
        await connection.disconnect()


async def reconnect(device: Device, addr: Address, mode: str) -> None:
    """Phase 2: reconnect using the stored bond. This is the assertion."""
    connection = await device.connect(addr, **connect_kwargs(mode))
    try:
        if mode == "le":
            LOG.info("reconnected, encrypting with stored LTK")
            await connection.encrypt()
        else:
            LOG.info("reconnected, authenticating with stored link key")
            await connection.authenticate()
            await connection.encrypt()
        LOG.info("re-established security from stored keys")
    finally:
        await connection.disconnect()


async def run(args) -> int:
    keystore = MemoryKeyStore()

    async with await open_transport(args.transport) as hci_transport:
        device = make_device(hci_transport, keystore)
        await device.power_on()

        if args.addr:
            addr = Address(args.addr, Address.PUBLIC_DEVICE_ADDRESS)
        else:
            LOG.info("scanning for '%s'...", args.name)
            try:
                addr = await discover_by_name(device, args.name, DISCOVERY_TIMEOUT)
            except asyncio.TimeoutError:
                LOG.error("no device advertising '%s' found in %.0fs",
                          args.name, DISCOVERY_TIMEOUT)
                LOG.error("Is the DUT running and advertising?")
                return 2

        LOG.info("--- phase 1: bond (%s) ---", args.mode)
        await asyncio.wait_for(bond(device, addr, args.mode), PHASE_TIMEOUT)

        # Bumble keys the store on str(peer_address), e.g. '2C:CF:67:BE:08:05/P'.
        # Just check the store is non-empty
        if not keystore.all_keys:
            LOG.error("peer retained no keys -- bonding did not complete")
            return 2
        LOG.info("peer holds keys for: %s", ", ".join(keystore.all_keys))
        await asyncio.sleep(SETTLE)

        LOG.info("--- phase 2: reconnect (%s) ---", args.mode)
        try:
            await asyncio.wait_for(reconnect(device, addr, args.mode), PHASE_TIMEOUT)
        except asyncio.TimeoutError:
            LOG.error("FAIL: reconnect timed out")
            return 1
        except Exception as exc:  # report whatever the stack raised
            LOG.error("FAIL: security not re-established from stored keys: %s", exc)
            if args.mode == "le":
                LOG.error("The DUT did not accept its own stored LTK.")
            else:
                LOG.error(
                    "Expected if the Pico replayed a corrupted link key "
                    "(Authentication Complete status 0x05, PIN or Key Missing)."
                )
            return 1

    LOG.info("PASS [%s]: bond survived a disconnect", args.mode)
    return 0


DESCRIPTION = """\
Check that a bond established with a Pico survives a disconnect.

  phase 1  connect, bond, disconnect
  phase 2  reconnect and re-establish security from the stored keys

Phase 2 BTstack frees the
hci_connection_t on disconnect, so the reconnect must be answered from the
flash-backed TLV database rather than the RAM cache.
"""

EPILOG = """\
examples:
  %(prog)s --addr 2C:CF:67:BE:08:05
  %(prog)s --addr 2C:CF:67:BE:08:05 --mode le -v
  %(prog)s --addr 2C:CF:67:BE:08:05 --transport hci-socket:hci0

exit codes:
  0  pass
  1  bond did not survive the disconnect
  2  setup or harness error (a page/accept timeout usually means the DUT is
     not running -- check its console for an assertion)
"""


def main() -> int:
    parser = argparse.ArgumentParser(
        description=DESCRIPTION,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument(
        "--addr",
        help="Pico BD_ADDR; btstack prints it at boot as 'BTstack up and running on ...'",
    )
    target.add_argument(
        "--name",
        help="find the DUT by scanning for this advertised local name instead "
        "(try 'PicoBondTest')",
    )
    parser.add_argument(
        "--transport",
        default="usb:0",
        help="Bumble transport for the peer radio (default: usb:0). "
        "Use hci-socket:hci0 to drive a built-in adapter (must be down).",
    )
    parser.add_argument(
        "--mode",
        choices=("ctkd", "classic", "le"),
        default="ctkd",
        help="ctkd: BR/EDR bond + CTKD over BR/EDR (default). "
        "classic: BR/EDR SSP bond only, no CTKD. "
        "le: LE SMP bond and re-encryption. "
        "Run all three to localise a failure.",
    )
    parser.add_argument("--flash", help="UF2 to erase+load before testing")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)-7s %(name)s: %(message)s",
    )

    if args.flash:
        try:
            flash_pico(args.flash)
        except subprocess.CalledProcessError as exc:
            LOG.error("flashing failed: %s", exc)
            return 2

    try:
        return asyncio.run(run(args))
    except Exception as exc:  # noqa: BLE001
        LOG.error("harness error: %s: %s", type(exc).__name__, exc or "<no message>")
        if args.verbose:
            LOG.error("", exc_info=True)
        return 2


if __name__ == "__main__":
    sys.exit(main())
