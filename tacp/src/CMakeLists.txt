cmake_minimum_required(VERSION 3.21)

add_library(tacp STATIC
	debug.h
	debug.c
	things_tiny_id.h
	things_tiny_id.c
	protocols.h
	tacp.h
	tacp.c
)
