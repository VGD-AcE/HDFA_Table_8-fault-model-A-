In order to achieve fast reading and writing of excel files, we use the ***libxl*** library in the project. The following is the specific configuration method:

The files uploaded in the repository already contain the ***include_cpp*** folder, ***lib64*** folder and ***libxl.dll*** file (in the ***x64/Release*** folder) necessary to load the ***libxl*** library.

******* Visual Studio environment configurations *******

（Visual studio 2022）Project – Properties

C/C++ - General - Additional Include Directories: Set the path to the ***include_cpp*** folder.

Linker - General - Additional Library Directories: Set the path to the ***lib64*** folder.

Linker - Input - Additional Dependencies: Fill in ***libxl.lib***.

Finally copy the ***libxl.dll*** file to the path where the ***.exe*** file is located (this project is ***x64/Release***).
