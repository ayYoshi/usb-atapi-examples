default:
	gcc -o build/main src/atapi.c src/usb.c src/main.c -lusb-1.0
