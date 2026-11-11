# Delete existing sig if present
file(REMOVE ${SIG})

# Picks which variant to use based on the environment variables set
if(DEFINED ENV{OPENSSL_PASS} AND DEFINED ENV{OPENSSL_KEY})
    # OpenSSL variant, for example with keys that require a password
    # Set the OPENSSL_PASS environment variable to the password to unlock the key,
    # and the OPENSSL_KEY environment variable to the path to the private key

    message(STATUS "Signing ${HASH} with OpenSSL")

    # Get public key
    execute_process(COMMAND
        openssl ec -in $ENV{OPENSSL_KEY} -out ${PUBKEY} -pubout -passin env:OPENSSL_PASS
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to read public key from $ENV{OPENSSL_KEY} using OpenSSL (command returned ${res})")
    endif()

    # Use openssl to sign
    execute_process(COMMAND
        openssl pkeyutl -in ${HASH} -inkey $ENV{OPENSSL_KEY} -out ${SIG} -pkeyopt digest:sha256 -passin env:OPENSSL_PASS
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to sign ${HASH} using OpenSSL (command returned ${res})")
    endif()
elseif(DEFINED ENV{PKCS11_PIN} AND DEFINED ENV{PKCS11_ID})
    # PKCS11 variant, for example with an HSM
    # Set the PKCS11_PIN environment variable to the pin to unlock the HSM,
    # and the PKCS11_ID environment variable to the key ID to use
    # Optionally set PKCS11_SLOT to the slot to use, if you have multiple

    message(STATUS "Signing ${HASH} with pkcs11")

    if (DEFINED ENV{PKCS11_SLOT})
        set(SLOT_ARG "--slot")
        set(SLOT_NUM "$ENV{PKCS11_SLOT}")
    endif()

    # Get public key from HSM
    execute_process(COMMAND
        pkcs11-tool --read-object --type pubkey ${SLOT_ARG} ${SLOT_NUM} --id $ENV{PKCS11_ID} --output-file ${PUBKEY}
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to read public key from ID $ENV{PKCS11_ID} using pkcs11-tool (command returned ${res})")
    endif()

    # Use pkcs11-tool to sign
    execute_process(COMMAND
        pkcs11-tool --sign ${SLOT_ARG} ${SLOT_NUM} --id $ENV{PKCS11_ID} --mechanism ECDSA --input-file ${HASH} --output-file ${SIG} --signature-format openssl --pin env:PKCS11_PIN
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to sign ${HASH} using pkcs11-tool (command returned ${res})")
    endif()
else()
    # Simple OpenSSL variant using example keys in SDK, which don't require a password
    # Used when the above environment variables aren't set, but should never be used in production

    message(STATUS "Signing ${HASH} with OpenSSL, using SDK example keys")

    # Copy across default public key
    configure_file(${CMAKE_CURRENT_LIST_DIR}/example_keys/public.pem ${PUBKEY} COPYONLY)

    # Warn whenever this default is used, as it's using the example key, unless example keys are allowed
    if (NOT ${ALLOW_EXAMPLE_KEYS})
        message(WARNING "You are using example key files for external signing - do not use these for production")
    endif()

    # Use openssl to sign
    execute_process(COMMAND
        openssl pkeyutl -in ${HASH} -inkey ${CMAKE_CURRENT_LIST_DIR}/example_keys/private.pem -out ${SIG} -pkeyopt digest:sha256
        RESULT_VARIABLE res
    )
    if (res)
        message(FATAL_ERROR "Failed to sign ${HASH} with example keys using OpenSSL (command returned ${res})")
    endif()
endif()
