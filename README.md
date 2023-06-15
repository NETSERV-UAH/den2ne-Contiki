Den2ne-Contiki
==========================

## Introducción

En este repositorio se encuentra la implementación del algoritmo utilizando la plataforma Contiki. En el directorio '4.8' se encuentran los proyectos para la versión 4.8 de Contiki. En él existen 2 implementaciones del algoritmo, una utilizando IPv6 (den2ne-ipv6) y otro solamente con las modificaciones sobre el funcionamiento respecto al código original.


## Instrucciones de uso

### Ejecutable

Para utilizar la implementación mediante IPv6 del nodo root, debemos ejecutar los siguientes comandos:

        cd ./4.8/examples/den2ne-ipv6/root/
        rm -rf build/ *.native
        make
        sudo ./iotorii-root.native

Para utilizar la implementación mediante IPv6 del nodo común, debemos ejecutar los siguientes comandos:

        cd ./4.8/examples/den2ne-ipv6/common-node/
        rm -rf build/ *.native
        make
        sudo ./iotorii-common.native

### Configuración de la red