3 bytes magic number -> 0x082C8BF1
4 byte uint32_t number of entries in the table
5 byte Entries defined above repeated the number of times defined above:
	4 byte uint32_t tableEntry size
	1 byte uint8_t tableEntry data
variable amount of encoded data:
	1 byte data length
	4 byte data dividend
