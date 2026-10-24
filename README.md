# La Academia Real de los Colores y las Formas

Actividad gráfica para niñas y niños de 3 a 4 años. La aplicación está escrita
en C++17 y usa la API gráfica nativa de Windows (Win32/GDI). El castillo, sus
personajes, las figuras y las recompensas se dibujan en pantalla. La aplicación
no requiere bibliotecas de audio externas.

## Compilar y ejecutar

Requisitos: Windows 10/11, CMake 3.20 o posterior y un
compilador C++ con el Windows SDK (por ejemplo, MinGW-w64 o Visual Studio 2022
con el componente «Desarrollo para el escritorio con C++»).

En PowerShell, desde esta carpeta:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\AcademiaReal.exe
```

Con Visual Studio instalado, también se puede abrir `CMakeLists.txt` directamente.

## Texto

Las preguntas, pistas e instrucciones aparecen escritas en pantalla. El juego
funciona sin sonido ni dependencias de audio.

## Actividades

El menú ofrece seis aventuras con seis preguntas cada una, una pantalla de
instrucciones y salida. Cada pregunta permite hasta tres intentos; equivocarse
no quita coronas. La actividad de vestir acepta tanto clics como arrastrar una
prenda sobre el personaje. El desafío mezcla colores, formas y cantidades.
