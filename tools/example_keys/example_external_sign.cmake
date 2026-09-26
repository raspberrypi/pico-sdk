# Delete existing sig if present
file(REMOVE ${SIG})

if(DEFINED ENV{OPENSSL_PASS} AND DEFINED ENV{OPENSSL_KEY})
    # OpenSSL variant, for example with keys that require a password
    # Set the OPENSSL_PASS environment variable to the password to unlock the key,
    # and the OPENSSL_KEY environment variable to the path to the private key

    message(STATUS "Signing ${HASH} with OpenSSL")

    # Get public key
    execute_process(COMMAND
        openssl ec -in $ENV{OPENSSL_KEY} -out ${PUBKEY} -pubout -passin env:OPENSSL_PASS
    )

    # Use openssl to sign
    execute_process(COMMAND
        openssl pkeyutl -in ${HASH} -inkey $ENV{OPENSSL_KEY} -out ${SIG} -pkeyopt digest:sha256 -passin env:OPENSSL_PASS
    )
elseif(DEFINED ENV{PKCS11_PIN})
    # PKCS11 variant, for example with an HSM
    # Set the PKCS11_PIN environment variable to the pin to unlock the HSM

    message(STATUS "Signing ${HASH} with pkcs11")

    # Get public key from HSM
    execute_process(COMMAND
        pkcs11-tool --read-object --type pubkey --id 1 --output-file ${PUBKEY}.der
    )
    # Convert to PEM format
    execute_process(COMMAND
        openssl pkey -pubin -inform DER -in ${PUBKEY}.der -out ${PUBKEY}
    )
    file(REMOVE ${PUBKEY}.der)

    # Use pkcs11-tool to sign
    execute_process(COMMAND
        pkcs11-tool --sign --id 1 --mechanism ECDSA --input-file ${HASH} --output-file ${SIG} --signature-format openssl --pin env:PKCS11_PIN
    )
else()
    # Simple OpenSSL variant using example keys in SDK, which don't require a password

    message(STATUS "Signing ${HASH} with OpenSSL, using SDK example keys")

    # Copy across default public key
    file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/public.pem ${PUBKEY})

    # Add any tests using this here - everything else gets a warning
    if (NOT "${HASH}" MATCHES "kitchen_sink")
        message(WARNING "Using example key files for external signing - do not use these for production")
    endif()

    # Use openssl to sign
    execute_process(COMMAND
        openssl pkeyutl -in ${HASH} -inkey ${CMAKE_CURRENT_LIST_DIR}/private.pem -out ${SIG} -pkeyopt digest:sha256
    )
endif()
