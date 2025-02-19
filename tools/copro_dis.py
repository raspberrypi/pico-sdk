#!/usr/bin/env python3

# NOTE THIS SCRIPT IS DEPRECATED. Use 'picotool coprodis' instead

import argparse, re

parser = argparse.ArgumentParser(description="Disassemble RCP instructions in DIS file")

parser.add_argument("input", help="Input DIS")
parser.add_argument("output", help="Output DIS")

args = parser.parse_args()

fin = open(args.input, mode="r")
contents = fin.read()
fin.close()

def gpiodir(val):
    val = int(val)
    if val//4 == 0:
        return "out"
    elif val//4 == 1:
        return "oe"
    elif val//4 == 2:
        return "in"
    else:
        return "unknown"

def gpiohilo(val):
    val = int(val)
    if val % 4 == 0:
        return "lo_" + gpiodir(val)
    elif val % 4 == 1:
        return "hi_" + gpiodir(val)
    else:
        return "unknown"

def gpiopxsc(val):
    val = int(val)
    if val == 0:
        return "put"
    elif val == 1:
        return "xor"
    elif val == 2:
        return "set"
    elif val == 3:
        return "clr"
    else:
        return "unknown"
    
def gpioxsc2(val):
    val = int(val)
    return gpiopxsc(val - 4) + ("2" if val > 4 else "")

def gpioxsc(val):
    val = int(val)
    return gpiopxsc(val - 4)

replacements = [
    # ========================== RCP ==========================
    (r'mrc\s*p?7, #?0, (.*), cr?(.*), cr?(.*), [\{#]1}?',   lambda m: 'rcp_canary_get {0}, 0x{1:02x} ({1}), delay'.format(m.group(1), int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mrc2\s*p?7, #?0, (.*), cr?(.*), cr?(.*), [\{#]1}?',  lambda m: 'rcp_canary_get {0}, 0x{1:02x} ({1}), nodelay'.format(m.group(1), int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mcr\s*p?7, #?0, (.*), cr?(.*), cr?(.*), [\{#]1}?',   lambda m: 'rcp_canary_check {0}, 0x{1:02x} ({1}), delay'.format(m.group(1), int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mcr2\s*p?7, #?0, (.*), cr?(.*), cr?(.*), [\{#]1}?',  lambda m: 'rcp_canary_check {0}, 0x{1:02x} ({1}), nodelay'.format(m.group(1), int(m.group(2)) * 16 + int(m.group(3)))),

    (r'mrc\s*p?7, #?1, (.*), cr?(.*), cr?(.*), [\{#]0}?',   r'rcp_canary_status \1, delay'),
    (r'mrc2\s*p?7, #?1, (.*), cr?(.*), cr?(.*), [\{#]0}?',   r'rcp_canary_status \1, nodelay'),
    (r'mcr\s*p?7, #?1, (.*), cr?(.*), cr?(.*), [\{#]0}?',   r'rcp_bvalid \1, delay'),
    (r'mcr2\s*p?7, #?1, (.*), cr?(.*), cr?(.*), [\{#]0}?',  r'rcp_bvalid \1, nodelay'),

    (r'mcr\s*p?7, #?2, (.*), cr?(.*), cr?(.*), [\{#]0}?',   r'rcp_btrue \1, delay'),
    (r'mcr2\s*p?7, #?2, (.*), cr?(.*), cr?(.*), [\{#]0}?',  r'rcp_btrue \1, nodelay'),

    (r'mcr\s*p?7, #?3, (.*), cr?(.*), cr?(.*), [\{#]1}?',   r'rcp_bfalse \1, delay'),
    (r'mcr2\s*p?7, #?3, (.*), cr?(.*), cr?(.*), [\{#]1}?',  r'rcp_bfalse \1, nodelay'),

    (r'mcr\s*p?7, #?4, (.*), cr?(.*), cr?(.*), [\{#]0}?',   lambda m: 'rcp_count_set 0x{0:02x} ({0}), delay'.format(int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mcr2\s*p?7, #?4, (.*), cr?(.*), cr?(.*), [\{#]0}?',  lambda m: 'rcp_count_set 0x{0:02x} ({0}), nodelay'.format(int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mcr\s*p?7, #?5, (.*), cr?(.*), cr?(.*), [\{#]1}?',   lambda m: 'rcp_count_check 0x{0:02x} ({0}), delay'.format(int(m.group(2)) * 16 + int(m.group(3)))),
    (r'mcr2\s*p?7, #?5, (.*), cr?(.*), cr?(.*), [\{#]1}?',  lambda m: 'rcp_count_check 0x{0:02x} ({0}), nodelay'.format(int(m.group(2)) * 16 + int(m.group(3)))),

    (r'mcrr\s*p?7, #?0, (.*), (.*), cr?(.*)',          r'rcp_b2valid \1, \2, delay'),
    (r'mcrr2\s*p?7, #?0, (.*), (.*), cr?(.*)',         r'rcp_b2valid \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?1, (.*), (.*), cr?(.*)',          r'rcp_b2and \1, \2, delay'),
    (r'mcrr2\s*p?7, #?1, (.*), (.*), cr?(.*)',         r'rcp_b2and \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?2, (.*), (.*), cr?(.*)',          r'rcp_b2or \1, \2, delay'),
    (r'mcrr2\s*p?7, #?2, (.*), (.*), cr?(.*)',         r'rcp_b2or \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?3, (.*), (.*), cr?(.*)',          r'rcp_bxorvalid \1, \2, delay'),
    (r'mcrr2\s*p?7, #?3, (.*), (.*), cr?(.*)',         r'rcp_bxorvalid \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?4, (.*), (.*), cr?(.*)',          r'rcp_bxortrue \1, \2, delay'),
    (r'mcrr2\s*p?7, #?4, (.*), (.*), cr?(.*)',         r'rcp_bxortrue \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?5, (.*), (.*), cr?(.*)',          r'rcp_bxorfalse \1, \2, delay'),
    (r'mcrr2\s*p?7, #?5, (.*), (.*), cr?(.*)',         r'rcp_bxorfalse \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?6, (.*), (.*), cr?(.*)',          r'rcp_ivalid \1, \2, delay'),
    (r'mcrr2\s*p?7, #?6, (.*), (.*), cr?(.*)',         r'rcp_ivalid \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?7, (.*), (.*), cr?(.*)',          r'rcp_iequal \1, \2, delay'),
    (r'mcrr2\s*p?7, #?7, (.*), (.*), cr?(.*)',         r'rcp_iequal \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?8, (.*), (.*), cr?0',             r'rcp_salt_core0 \1, \2, delay'),
    (r'mcrr2\s*p?7, #?8, (.*), (.*), cr?0',            r'rcp_salt_core0 \1, \2, nodelay'),

    (r'mcrr\s*p?7, #?8, (.*), (.*), cr?1',             r'rcp_salt_core1 \1, \2, delay'),
    (r'mcrr2\s*p?7, #?8, (.*), (.*), cr?1',            r'rcp_salt_core1 \1, \2, nodelay'),

    (r'cdp\s*p?7, #?0, cr?0, cr?0, cr?0, [\{#]1}?',          r'rcp_panic'),

    # ========================== DCP ==========================

    ('([0-9a-f]{8}:\tee00 0400 \t).*',r'\1dcp_init'),
    ('([0-9a-f]{8}:\tee00 0500 \t).*',r'\1dcps_init'),
    ('([0-9a-f]{8}:\tee00 0401 \t).*',r'\1dcp_add0'),
    ('([0-9a-f]{8}:\tee00 0501 \t).*',r'\1dcps_add0'),
    ('([0-9a-f]{8}:\tee10 0401 \t).*',r'\1dcp_add1'),
    ('([0-9a-f]{8}:\tee10 0501 \t).*',r'\1dcps_add1'),
    ('([0-9a-f]{8}:\tee10 0421 \t).*',r'\1dcp_sub1'),
    ('([0-9a-f]{8}:\tee10 0521 \t).*',r'\1dcps_sub1'),
    ('([0-9a-f]{8}:\tee20 0401 \t).*',r'\1dcp_sqr0'),
    ('([0-9a-f]{8}:\tee20 0501 \t).*',r'\1dcps_sqr0'),
    ('([0-9a-f]{8}:\tee80 0402 \t).*',r'\1dcp_norm'),
    ('([0-9a-f]{8}:\tee80 0502 \t).*',r'\1dcps_norm'),
    ('([0-9a-f]{8}:\tee80 0422 \t).*',r'\1dcp_nrdf'),
    ('([0-9a-f]{8}:\tee80 0522 \t).*',r'\1dcps_nrdf'),
    ('([0-9a-f]{8}:\tee80 0420 \t).*',r'\1dcp_nrdd'),
    ('([0-9a-f]{8}:\tee80 0520 \t).*',r'\1dcps_nrdd'),
    ('([0-9a-f]{8}:\tee80 0440 \t).*',r'\1dcp_ntdc'),
    ('([0-9a-f]{8}:\tee80 0540 \t).*',r'\1dcps_ntdc'),
    ('([0-9a-f]{8}:\tee80 0460 \t).*',r'\1dcp_nrdc'),
    ('([0-9a-f]{8}:\tee80 0560 \t).*',r'\1dcps_nrdc'),
    ('([0-9a-f]{8}:\tec4(.) (.)400 \t).*',r'\1dcp_wxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)500 \t).*',r'\1dcps_wxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)401 \t).*',r'\1dcp_wymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)501 \t).*',r'\1dcps_wymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)402 \t).*',r'\1dcp_wefd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)502 \t).*',r'\1dcps_wefd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)410 \t).*',r'\1dcp_wxup _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)510 \t).*',r'\1dcps_wxup _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)411 \t).*',r'\1dcp_wyup _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)511 \t).*',r'\1dcps_wyup _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)412 \t).*',r'\1dcp_wxyu _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)512 \t).*',r'\1dcps_wxyu _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)420 \t).*',r'\1dcp_wxms _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)520 \t).*',r'\1dcps_wxms _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)430 \t).*',r'\1dcp_wxmo _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)530 \t).*',r'\1dcps_wxmo _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)440 \t).*',r'\1dcp_wxdd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)540 \t).*',r'\1dcps_wxdd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)450 \t).*',r'\1dcp_wxdq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)550 \t).*',r'\1dcps_wxdq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)460 \t).*',r'\1dcp_wxuc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)560 \t).*',r'\1dcps_wxuc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)470 \t).*',r'\1dcp_wxic _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)570 \t).*',r'\1dcps_wxic _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)480 \t).*',r'\1dcp_wxdc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)580 \t).*',r'\1dcps_wxdc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)492 \t).*',r'\1dcp_wxfc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)592 \t).*',r'\1dcps_wxfc _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)4a0 \t).*',r'\1dcp_wxfm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)5a0 \t).*',r'\1dcps_wxfm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)4b0 \t).*',r'\1dcp_wxfd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)5b0 \t).*',r'\1dcps_wxfd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)4c0 \t).*',r'\1dcp_wxfq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec4(.) (.)5c0 \t).*',r'\1dcps_wxfq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)410 \t).*',r'\1dcp_rxvd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)510 \t).*',r'\1dcps_rxvd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)430 \t).*',r'\1dcp_rcmp _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)530 \t).*',r'\1dcps_rcmp _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)412 \t).*',r'\1dcp_rdfa _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)512 \t).*',r'\1dcps_rdfa _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)432 \t).*',r'\1dcp_rdfs _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)532 \t).*',r'\1dcps_rdfs _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)452 \t).*',r'\1dcp_rdfm _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)552 \t).*',r'\1dcps_rdfm _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)472 \t).*',r'\1dcp_rdfd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)572 \t).*',r'\1dcps_rdfd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)492 \t).*',r'\1dcp_rdfq _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)592 \t).*',r'\1dcps_rdfq _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)4b2 \t).*',r'\1dcp_rdfg _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)5b2 \t).*',r'\1dcps_rdfg _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)413 \t).*',r'\1dcp_rdic _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)513 \t).*',r'\1dcps_rdic _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)433 \t).*',r'\1dcp_rduc _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tee10 (.)533 \t).*',r'\1dcps_rduc _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)408 \t).*',r'\1dcp_rxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)508 \t).*',r'\1dcps_rxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)409 \t).*',r'\1dcp_rymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)509 \t).*',r'\1dcps_rymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)40a \t).*',r'\1dcp_refd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)50a \t).*',r'\1dcps_refd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)4(.)4 \t).*',r'\1dcp_rxms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tec5(.) (.)5(.)4 \t).*',r'\1dcps_rxms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tec5(.) (.)4(.)5 \t).*',r'\1dcp_ryms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tec5(.) (.)5(.)5 \t).*',r'\1dcps_ryms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tec5(.) (.)411 \t).*',r'\1dcp_rxyh _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)511 \t).*',r'\1dcps_rxyh _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)421 \t).*',r'\1dcp_rymr _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)521 \t).*',r'\1dcps_rymr _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)441 \t).*',r'\1dcp_rxmq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)541 \t).*',r'\1dcps_rxmq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)410 \t).*',r'\1dcp_rdda _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)510 \t).*',r'\1dcps_rdda _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)430 \t).*',r'\1dcp_rdds _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)530 \t).*',r'\1dcps_rdds _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)450 \t).*',r'\1dcp_rddm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)550 \t).*',r'\1dcps_rddm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)470 \t).*',r'\1dcp_rddd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)570 \t).*',r'\1dcps_rddd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)490 \t).*',r'\1dcp_rddq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)590 \t).*',r'\1dcps_rddq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)4b0 \t).*',r'\1dcp_rddg _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tec5(.) (.)5b0 \t).*',r'\1dcps_rddg _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)410 \t).*',r'\1dcp_pxvd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)510 \t).*',r'\1dcps_pxvd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)430 \t).*',r'\1dcp_pcmp _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)530 \t).*',r'\1dcps_pcmp _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)412 \t).*',r'\1dcp_pdfa _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)512 \t).*',r'\1dcps_pdfa _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)432 \t).*',r'\1dcp_pdfs _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)532 \t).*',r'\1dcps_pdfs _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)452 \t).*',r'\1dcp_pdfm _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)552 \t).*',r'\1dcps_pdfm _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)472 \t).*',r'\1dcp_pdfd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)572 \t).*',r'\1dcps_pdfd _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)492 \t).*',r'\1dcp_pdfq _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)592 \t).*',r'\1dcps_pdfq _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)4b2 \t).*',r'\1dcp_pdfg _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)5b2 \t).*',r'\1dcps_pdfg _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)413 \t).*',r'\1dcp_pdic _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)513 \t).*',r'\1dcps_pdic _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)433 \t).*',r'\1dcp_pduc _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfe10 (.)533 \t).*',r'\1dcps_pduc _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)408 \t).*',r'\1dcp_pxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)508 \t).*',r'\1dcps_pxmd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)409 \t).*',r'\1dcp_pymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)509 \t).*',r'\1dcps_pymd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)40a \t).*',r'\1dcp_pefd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)50a \t).*',r'\1dcps_pefd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)4(.)4 \t).*',r'\1dcp_pxms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tfc5(.) (.)5(.)4 \t).*',r'\1dcps_pxms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tfc5(.) (.)4(.)5 \t).*',r'\1dcp_pyms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tfc5(.) (.)5(.)5 \t).*',r'\1dcps_pyms _cpu_reg_\3_, _cpu_reg_\2_, #0x\4'),
    ('([0-9a-f]{8}:\tfc5(.) (.)411 \t).*',r'\1dcp_pxyh _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)511 \t).*',r'\1dcps_pxyh _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)421 \t).*',r'\1dcp_pymr _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)521 \t).*',r'\1dcps_pymr _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)441 \t).*',r'\1dcp_pxmq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)541 \t).*',r'\1dcps_pxmq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)410 \t).*',r'\1dcp_pdda _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)510 \t).*',r'\1dcps_pdda _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)430 \t).*',r'\1dcp_pdds _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)530 \t).*',r'\1dcps_pdds _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)450 \t).*',r'\1dcp_pddm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)550 \t).*',r'\1dcps_pddm _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)470 \t).*',r'\1dcp_pddd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)570 \t).*',r'\1dcps_pddd _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)490 \t).*',r'\1dcp_pddq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)590 \t).*',r'\1dcps_pddq _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)4b0 \t).*',r'\1dcp_pddg _cpu_reg_\3_, _cpu_reg_\2_'),
    ('([0-9a-f]{8}:\tfc5(.) (.)5b0 \t).*',r'\1dcps_pddg _cpu_reg_\3_, _cpu_reg_\2_'),
    ('_cpu_reg_([0-9])_', r'r\1'),
    ('_cpu_reg_a_', r'sl'),
    ('_cpu_reg_b_', r'fp'),
    ('_cpu_reg_c_', r'ip'),
    ('_cpu_reg_d_', r'sp'),
    ('_cpu_reg_e_', r'lr'),
    ('_cpu_reg_f_', r'pc'),

    # ========================== GPIO ==========================

    # OUT and OE mask write instructions
    (r'mcr\s*p?0, #?([0-3]), (.*), cr?0, cr?([0145])', lambda m: 'gpioc_{0}_{1} {2}'.format(
        gpiohilo(m.group(3)), gpiopxsc(m.group(1)), m.group(2)
    )),
    (r'mcrr\s*p?0, #?([0-3]), (.*), (.*), cr?([04])', lambda m: 'gpioc_hilo_{0}_{1} {2}, {3}'.format(
        gpiodir(m.group(4)), gpiopxsc(m.group(1)), m.group(2), m.group(3)
    )),
    # Single-bit write instructions
    (r'mcrr\s*p?0, #?([4-7]), (.*), (.*), cr?([04])', lambda m: 'gpioc_bit_{0}_{1} {2}, {3}'.format(
        gpiodir(m.group(4)), gpioxsc2(m.group(1)), m.group(2), m.group(3)
    )),
    (r'mcr\s*p?0, #?([5-7]), (.*), cr?0, cr?([04])', lambda m: 'gpioc_bit_{0}_{1} {2}'.format(
        gpiodir(m.group(3)), gpioxsc(m.group(1)), m.group(2)
    )),
    # Indexed mask write instructions -- write to a dynamically selected 32-bit
    (r'mcrr\s*p?0, #?(8|9|10|11), (.*), (.*), cr?([04])', lambda m: 'gpioc_index_{0}_{1} {2}, {3}'.format(
        gpiodir(m.group(4)), gpiopxsc(int(m.group(1)) - 8), m.group(2), m.group(3)
    )),
    # Read instructions
    (r'mrc\s*p?0, #?0, (.*), cr?0, cr?([014589])', lambda m: 'gpioc_{0}_get {1}'.format(
        gpiohilo(m.group(2)), m.group(1)
    )),
    (r'mrrc\s*p?0, #?0, (.*), (.*), cr?([048])', lambda m: 'gpioc_hilo_{0}_get {1}, {2}'.format(
        gpiodir(m.group(3)), m.group(1), m.group(2)
    )),
]

# Add clang DCP replacements
for pat, rep in replacements:
    if pat.startswith('([0-9a-f]{8}:\t'):
        mid = pat.split('\t')[1]
        left, right = mid.split(' ')[0:2]
        if len(right) > 6:
            new_pat = f"([0-9a-f]{{8}}:\s*{left[2:]} {left[0:2]} {right[4:]} {right[:4]} \s*).*"
            new_rep = rep.replace('3', '7')
            new_rep = new_rep.replace('4', '3')
            new_rep = new_rep.replace('7', '4')
            replacements.append((new_pat, new_rep))
        else:
            replacements.append((f"([0-9a-f]{{8}}:\s*{left[2:]} {left[0:2]} {right[-2:]} {right[:-2]} \s*).*", rep))

for pat, rep in replacements:
    contents = re.sub(pat, rep, contents)

fout = open(args.output, mode="w")
fout.write(contents)
fout.close()
