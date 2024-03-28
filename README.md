Den2ne-Contiki / MuHoW
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

Raspberry 1: 
- Contiki: fd03::…:708   &emsp; (~/den2ne-Contiki/4.8/arch/platform/native/platform.c:120)
- tun0: fd03::2          &emsp;&emsp;&emsp;&emsp;&ensp; (~/den2ne-Contiki/4.8/arch/cpu/native/net/tun6net.c:70)
- wlan0: fd03::20        &emsp;&emsp;&emsp;&nbsp; (archivo /etc/network/interfaces)

Raspberry 2: 
- Contiki: fd03::…:709   &emsp; (~/den2ne-Contiki/4.8/arch/platform/native/platform.c:120)
- tun0: fd03::3          &emsp;&emsp;&emsp;&emsp;&ensp; (~/den2ne-Contiki/4.8/arch/cpu/native/net/tun6net.c:70)
- wlan0: fd03::30        &emsp;&emsp;&emsp;&nbsp; (archivo /etc/network/interfaces)
