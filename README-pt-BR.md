<<<<<<< Updated upstream
# Raspberry Pi Pico SDK

***O Raspberry Pi Pico SDK (a partir daqui SDK) provê os cabeçalhos, bibliotecas e um sistema de bulding necessário para escrever programas para dispositivos baseados em microcontroladores da série RP, assim como o Raspberry Pi Pico ou Raspberry Pi pico 2 em C, C++ ou em linguagem assembly.***

The Raspberry Pi Pico SDK (henceforth the SDK) provides the headers, libraries and build system
necessary to write programs for the RP-series microcontroller-based devices such as the Raspberry Pi Pico or Raspberry Pi Pico 2
in C, C++ or assembly language.

***O SDK é desenhado para prover uma API e ambiente de programação que é familiar tanto para os desenvolvedores de sistemas embarcados em C quanto para os outros desenvolvedores em C. Um único programa roda em um dispositivo por vez e começa com um método `main()` convencional. As bibliotecas padrão em C/C++ são compatíveis, assim como as bibliotecas e APIs de baixo nível em C para acessar todos o hardware dos microcontroladores da série RP incluindo PIO(IO Programada).***

The SDK is designed to provide an API and programming environment that is familiar both to non-embedded C developers and embedded C developers alike.
A single program runs on the device at a time and starts with a conventional `main()` method. Standard C/C++ libraries are supported along with
C-level libraries/APIs for accessing all of the RP-series microcontroller's hardware including PIO (Programmable IO).

***Adicionalmente, o SDK disponibiliza bibliotecas de alto nível para lidar com temporizadores, sincronzação, Wi-Fi e conexão Bluetooth, USB e programação multinúcleo. Essas bibliotecas devem ser compreeensivas o suficiente para que sua aplicação raramente, somente se for o caso, precise acessar os registradores do hardware diretamente. Entretanto, se você precisar ou preferir acessar os registradores de hardware puro, você também vai achar cabeçalhos de definição de registros completos e totalmente comentado no SDK. Não precisa procurar por isso no datasheet.***

Additionally, the SDK provides higher level libraries for dealing with timers, synchronization, Wi-Fi and Bluetooth networking, USB and multicore programming. These libraries should be comprehensive enough that your application code rarely, if at all, needs to access hardware registers directly. However, if you do need or prefer to access the raw hardware registers, you will also find complete and fully-commented register definition headers in the SDK. There's no need to look up addresses in the datasheet.

***O SDK pode ser usado para criar qualquer coisa desde simples aplicações, ambientes de execução completos como o MicroPython, até softwares de baixo nível, como o prórpio bootrom on-chip do microcontrolador da série RP.***

The SDK can be used to build anything from simple applications, fully-fledged runtime environments such as MicroPython, to low level software
such as the RP-series microcontroller's on-chip bootrom itself.

***O objetivo do design de todo o SDK é ser simples, mas poderoso.***

The design goal for entire SDK is to be simple but powerful.

***Bibliotecas/API adicionais que ainda não estão prontas para inclusão no SDK podem ser encontradas em [pico-extras](https://github.com/raspberrypi/pico-extras).***

Additional libraries/APIs that are not yet ready for inclusion in the SDK can be found in [pico-extras](https://github.com/raspberrypi/pico-extras).

# ***Documentação***

# Documentation

***Consulte [Instrodução ao Raspberry Pi Pico-Series](https://rptl.io/pico-get-started) para obter informações sobre como configurar seu hardware, IDE/ambiente e como criar e depurar software para a Raspberry Pi Pico e outros dispositivos baseados em microcontroladores da série RP***

See [Getting Started with the Raspberry Pi Pico-Series](https://rptl.io/pico-get-started) for information on how to setup your
hardware, IDE/environment and how to build and debug software for the Raspberry Pi Pico and other RP-series microcontroller based devices.

***Consulte [Conectando-se à Internet com o Raspberry Pi Pico W](https://rptl.io/picow-connect) para saber mais sobre como escrever aplicativos para seu Raspberry Pi Pico W que conectam a internet***

See [Connecting to the Internet with Raspberry Pi Pico W](https://rptl.io/picow-connect) to learn more about writing
applications for your Raspberry Pi Pico W that connect to the internet.

***Consulte [o Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk) para saber mais sobre programação usando o SDK, explorar recursos mais avançados e obter a documentação completa da API em PDF***

See [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk) to learn more about programming using the
SDK, to explore more advanced features, and for complete PDF-based API documentation.

***Consulte [a documentação da API do Raspberry Pi Pico SDK online](https://rptl.io/pico-doxygen) para obter a documentação da API baseada em HTML***

See [Online Raspberry Pi Pico SDK API docs](https://rptl.io/pico-doxygen) for HTML-based API documentation.

# Código de exemplo

# Example code

***Veja [pico-examples](https://github.com/raspberrypi/pico-examples) para exemplos de código que você pode criar***

See [pico-examples](https://github.com/raspberrypi/pico-examples) for example code you can build.

# Obtendo o código SDK mais recente

# Getting the latest SDK code

***A ramificação [master](https://github.com/raspberrypi/pico-sdk/tree/master/) do `pico-sdk` no GitHub contém a _versão estável mais recente_ do SDK. Se precisar ou quiser testar recursos futuos, você pode experimentar a ramificação [develop](https://github.com/raspberrypi/pico-sdk/tree/develop/)***

The [master](https://github.com/raspberrypi/pico-sdk/tree/master/) branch of `pico-sdk` on GitHub contains the 
_latest stable release_ of the SDK. If you need or want to test upcoming features, you can try the
[develop](https://github.com/raspberrypi/pico-sdk/tree/develop/) branch instead.

# Inicie seu próprio projeto

# Quick-start your own project

## Usando Visual Studio Code

## Using Visual Studio Code

***Você pode instalar a [extensão Raspberry Pi Pico Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) no VSCode***

You can install the [Raspberry Pi Pico Visual Studio Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) in VS Code.

## Linha de comando Unix

## Unix command line

***Essas instruções são extremamente concisas e baseada apenas em Linux. Para etapas detalhadas, instruções para outras plataformas e informações gerais, recomendamos que você consulte [o SDK C/C++ da série Raspberry Pi Pico](https://rptl.io/pico-c-sdk)***

These instructions are extremely terse, and Linux-based only. For detailed steps,
instructions for other platforms, and just in general, we recommend you see [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk)

1. Install CMake (at least version 3.13), python 3, a native compiler, and a GCC cross compiler
   ```
   sudo apt install cmake python3 build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
   ```
1. Set up your project to point to use the Raspberry Pi Pico SDK

   * Either by cloning the SDK locally (most common) :
      1. `git clone` this Raspberry Pi Pico SDK repository
      1. Copy [pico_sdk_import.cmake](https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake)
         from the SDK into your project directory
      2. Set `PICO_SDK_PATH` to the SDK location in your environment, or pass it (`-DPICO_SDK_PATH=`) to cmake later.
      3. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13...3.27)

          # initialize the SDK based on PICO_SDK_PATH
          # note: this must happen before project()
          include(pico_sdk_import.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or with the Raspberry Pi Pico SDK as a submodule :
      1. Clone the SDK as a submodule called `pico-sdk`
      1. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13...3.27)

          # initialize pico-sdk from submodule
          # note: this must happen before project()
          include(pico-sdk/pico_sdk_init.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or with automatic download from GitHub :
      1. Copy [pico_sdk_import.cmake](https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake)
         from the SDK into your project directory
      1. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13)

          # initialize pico-sdk from GIT
          # (note this can come from environment, CMake cache etc)
          set(PICO_SDK_FETCH_FROM_GIT on)

          # pico_sdk_import.cmake is a single file copied from this SDK
          # note: this must happen before project()
          include(pico_sdk_import.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or by cloning the SDK locally, but without copying `pico_sdk_import.cmake`:
       1. `git clone` this Raspberry Pi Pico SDK repository
       2. Setup a `CMakeLists.txt` like:

           ```cmake
           cmake_minimum_required(VERSION 3.13)
 
           # initialize the SDK directly
           include(/path/to/pico-sdk/pico_sdk_init.cmake)
 
           project(my_project)
 
           # initialize the Raspberry Pi Pico SDK
           pico_sdk_init()
 
           # rest of your project
 
           ```
1. Write your code (see [pico-examples](https://github.com/raspberrypi/pico-examples) or the [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk) documentation for more information)

   About the simplest you can do is a single source file (e.g. hello_world.c)

   ```c
   #include <stdio.h>
   #include "pico/stdlib.h"

   int main() {
       stdio_init_all();
       printf("Hello, world!\n");
       return 0;
   }
   ```
   And add the following to your `CMakeLists.txt`:

   ```cmake
   add_executable(hello_world
       hello_world.c
   )

   # Add pico_stdlib library which aggregates commonly used features
   target_link_libraries(hello_world pico_stdlib)

   # create map/bin/hex/uf2 file in addition to ELF.
   pico_add_extra_outputs(hello_world)
   ```

   Note this example uses the default UART for _stdout_;
   if you want to use the default USB see the [hello-usb](https://github.com/raspberrypi/pico-examples/tree/master/hello_world/usb) example.

1. Setup a CMake build directory.
      For example, if not using an IDE:
      ```
      $ mkdir build
      $ cd build
      $ cmake ..
      ```   
   
   When building for a board other than the Raspberry Pi Pico, you should pass `-DPICO_BOARD=board_name` to the `cmake` command above, e.g. `cmake -DPICO_BOARD=pico2 ..` or `cmake -DPICO_BOARD=pico_w ..` to configure the SDK and build options accordingly for that particular board.

   Specifying `PICO_BOARD=<boardname>` sets up various compiler defines (e.g. default pin numbers for UART and other hardware) and in certain 
   cases also enables the use of additional libraries (e.g. wireless support when building for `PICO_BOARD=pico_w`) which cannot
   be built without a board which provides the requisite hardware functionality.

   For a list of boards defined in the SDK itself, look in [this directory](src/boards/include/boards) which has a 
   header for each named board.

1. Make your target from the build directory you created.
      ```sh
      $ make hello_world
      ```

1. You now have `hello_world.elf` to load via a debugger, or `hello_world.uf2` that can be installed and run on your Raspberry Pi Pico-series device via drag and drop.

# RISC-V support on RP2350

See [Raspberry Pi Pico-series C/C++ SDK](https://rptl.io/pico-c-sdk) for information on setting up a build environment for RISC-V on RP2350.
=======
# Raspberry Pi Pico SDK

`O Raspberry Pi Pico SDK (a partir daqui SDK) provê os cabeçalhos, bibliotecas e um sistema de bulding necessário para escrever programas para dispositivos baseados em microcontroladores da série RP, assim como o Raspberry Pi Pico ou Raspberry Pi pico 2 em C, C++ ou em linguagem assembly.`

The Raspberry Pi Pico SDK (henceforth the SDK) provides the headers, libraries and build system
necessary to write programs for the RP-series microcontroller-based devices such as the Raspberry Pi Pico or Raspberry Pi Pico 2
in C, C++ or assembly language.

`O SDK é desenhado para prover uma API e ambiente de programação que é familiar tanto para os desenvolvedores de sistemas embarcados em C quanto para os outros desenvolvedores em C. Um único programa roda em um dispositivo por vez e começa com um método `main()` convencional. As bibliotecas padrão em C/C++ são compatíveis, assim como as bibliotecas e APIs de baixo nível em C para acessar todos o hardware dos microcontroladores da série RP incluindo PIO(IO Programada).`

The SDK is designed to provide an API and programming environment that is familiar both to non-embedded C developers and embedded C developers alike.
A single program runs on the device at a time and starts with a conventional `main()` method. Standard C/C++ libraries are supported along with
C-level libraries/APIs for accessing all of the RP-series microcontroller's hardware including PIO (Programmable IO).

`Adicionalmente, o SDK disponibiliza bibliotecas de alto nível para lidar com temporizadores, sincronzação, Wi-Fi e conexão Bluetooth, USB e programação multinúcleo. Essas bibliotecas devem ser compreeensivas o suficiente para que sua aplicação raramente, somente se for o caso, precise acessar os registradores do hardware diretamente. Entretanto, se você precisar ou preferir acessar os registradores de hardware puro, você também vai achar cabeçalhos de definição de registros completos e totalmente comentado no SDK. Não precisa procurar por isso no datasheet.`

Additionally, the SDK provides higher level libraries for dealing with timers, synchronization, Wi-Fi and Bluetooth networking, USB and multicore programming. These libraries should be comprehensive enough that your application code rarely, if at all, needs to access hardware registers directly. However, if you do need or prefer to access the raw hardware registers, you will also find complete and fully-commented register definition headers in the SDK. There's no need to look up addresses in the datasheet.

`O SDK pode ser usado para criar`

The SDK can be used to build anything from simple applications, fully-fledged runtime environments such as MicroPython, to low level software
such as the RP-series microcontroller's on-chip bootrom itself.

The design goal for entire SDK is to be simple but powerful.

Additional libraries/APIs that are not yet ready for inclusion in the SDK can be found in [pico-extras](https://github.com/raspberrypi/pico-extras).

# Documentation

See [Getting Started with the Raspberry Pi Pico-Series](https://rptl.io/pico-get-started) for information on how to setup your
hardware, IDE/environment and how to build and debug software for the Raspberry Pi Pico and other RP-series microcontroller based devices.

See [Connecting to the Internet with Raspberry Pi Pico W](https://rptl.io/picow-connect) to learn more about writing
applications for your Raspberry Pi Pico W that connect to the internet.

See [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk) to learn more about programming using the
SDK, to explore more advanced features, and for complete PDF-based API documentation.

See [Online Raspberry Pi Pico SDK API docs](https://rptl.io/pico-doxygen) for HTML-based API documentation.

# Example code

See [pico-examples](https://github.com/raspberrypi/pico-examples) for example code you can build.

# Getting the latest SDK code

The [master](https://github.com/raspberrypi/pico-sdk/tree/master/) branch of `pico-sdk` on GitHub contains the 
_latest stable release_ of the SDK. If you need or want to test upcoming features, you can try the
[develop](https://github.com/raspberrypi/pico-sdk/tree/develop/) branch instead.

# Quick-start your own project

## Using Visual Studio Code

You can install the [Raspberry Pi Pico Visual Studio Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) in VS Code.

## Unix command line

These instructions are extremely terse, and Linux-based only. For detailed steps,
instructions for other platforms, and just in general, we recommend you see [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk)

1. Install CMake (at least version 3.13), python 3, a native compiler, and a GCC cross compiler
   ```
   sudo apt install cmake python3 build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
   ```
1. Set up your project to point to use the Raspberry Pi Pico SDK

   * Either by cloning the SDK locally (most common) :
      1. `git clone` this Raspberry Pi Pico SDK repository
      1. Copy [pico_sdk_import.cmake](https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake)
         from the SDK into your project directory
      2. Set `PICO_SDK_PATH` to the SDK location in your environment, or pass it (`-DPICO_SDK_PATH=`) to cmake later.
      3. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13...3.27)

          # initialize the SDK based on PICO_SDK_PATH
          # note: this must happen before project()
          include(pico_sdk_import.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or with the Raspberry Pi Pico SDK as a submodule :
      1. Clone the SDK as a submodule called `pico-sdk`
      1. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13...3.27)

          # initialize pico-sdk from submodule
          # note: this must happen before project()
          include(pico-sdk/pico_sdk_init.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or with automatic download from GitHub :
      1. Copy [pico_sdk_import.cmake](https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake)
         from the SDK into your project directory
      1. Setup a `CMakeLists.txt` like:

          ```cmake
          cmake_minimum_required(VERSION 3.13)

          # initialize pico-sdk from GIT
          # (note this can come from environment, CMake cache etc)
          set(PICO_SDK_FETCH_FROM_GIT on)

          # pico_sdk_import.cmake is a single file copied from this SDK
          # note: this must happen before project()
          include(pico_sdk_import.cmake)

          project(my_project)

          # initialize the Raspberry Pi Pico SDK
          pico_sdk_init()

          # rest of your project

          ```

   * Or by cloning the SDK locally, but without copying `pico_sdk_import.cmake`:
       1. `git clone` this Raspberry Pi Pico SDK repository
       2. Setup a `CMakeLists.txt` like:

           ```cmake
           cmake_minimum_required(VERSION 3.13)
 
           # initialize the SDK directly
           include(/path/to/pico-sdk/pico_sdk_init.cmake)
 
           project(my_project)
 
           # initialize the Raspberry Pi Pico SDK
           pico_sdk_init()
 
           # rest of your project
 
           ```
1. Write your code (see [pico-examples](https://github.com/raspberrypi/pico-examples) or the [Raspberry Pi Pico-Series C/C++ SDK](https://rptl.io/pico-c-sdk) documentation for more information)

   About the simplest you can do is a single source file (e.g. hello_world.c)

   ```c
   #include <stdio.h>
   #include "pico/stdlib.h"

   int main() {
       stdio_init_all();
       printf("Hello, world!\n");
       return 0;
   }
   ```
   And add the following to your `CMakeLists.txt`:

   ```cmake
   add_executable(hello_world
       hello_world.c
   )

   # Add pico_stdlib library which aggregates commonly used features
   target_link_libraries(hello_world pico_stdlib)

   # create map/bin/hex/uf2 file in addition to ELF.
   pico_add_extra_outputs(hello_world)
   ```

   Note this example uses the default UART for _stdout_;
   if you want to use the default USB see the [hello-usb](https://github.com/raspberrypi/pico-examples/tree/master/hello_world/usb) example.

1. Setup a CMake build directory.
      For example, if not using an IDE:
      ```
      $ mkdir build
      $ cd build
      $ cmake ..
      ```   
   
   When building for a board other than the Raspberry Pi Pico, you should pass `-DPICO_BOARD=board_name` to the `cmake` command above, e.g. `cmake -DPICO_BOARD=pico2 ..` or `cmake -DPICO_BOARD=pico_w ..` to configure the SDK and build options accordingly for that particular board.

   Specifying `PICO_BOARD=<boardname>` sets up various compiler defines (e.g. default pin numbers for UART and other hardware) and in certain 
   cases also enables the use of additional libraries (e.g. wireless support when building for `PICO_BOARD=pico_w`) which cannot
   be built without a board which provides the requisite hardware functionality.

   For a list of boards defined in the SDK itself, look in [this directory](src/boards/include/boards) which has a 
   header for each named board.

1. Make your target from the build directory you created.
      ```sh
      $ make hello_world
      ```

1. You now have `hello_world.elf` to load via a debugger, or `hello_world.uf2` that can be installed and run on your Raspberry Pi Pico-series device via drag and drop.

# RISC-V support on RP2350

See [Raspberry Pi Pico-series C/C++ SDK](https://rptl.io/pico-c-sdk) for information on setting up a build environment for RISC-V on RP2350.
>>>>>>> Stashed changes
