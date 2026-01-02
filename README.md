# MiniShell en C

Este proyecto consiste en una implementación personalizada de un intérprete de comandos (shell). Se enfoca en el uso de llamadas al sistema de bajo nivel para la gestión de procesos y el control directo de la terminal.

## Estructura del Proyecto
Para facilitar la colaboración, el código se ha organizado de forma modular:
* `main.c`: Contiene el ciclo principal del shell.
* `lib/`: Contiene los módulos de soporte.

## Compilación
Para compilar tanto el archivo principal junto con todos los módulos, introduce el siguiente comando:
`gcc minishell.c lib/*.c -o main`

## Ejecución
Luego de que la compilación términe sin fallas, ejecuta el archivo generado en el punto anterior:
`./main`

## Importante
Antes de tocar `shell-history.c` asegúrate de entender como funciona `termios.h`, ya que un error aquí puede hacer que la terminal principal (no el minishell) deje de mostrar texto.

# Happy Coding!
