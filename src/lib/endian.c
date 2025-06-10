#include <arpa/inet.h>
#include <stdint.h>

uint64_t hton64(uint64_t value) {
	uint32_t high_bits = htonl((uint32_t)(value >> 32));
	uint32_t low_bits = htonl((uint32_t)(value & 0xffffffff));
	return (((uint64_t)low_bits) << 32) | high_bits;
}

uint64_t ntoh64(uint64_t value) {
	uint32_t high_bits = ntohl((uint32_t)(value >> 32));
	uint32_t low_bits = ntohl((uint32_t)(value & 0xffffffff));
	return (((uint64_t)low_bits) << 32) | high_bits;
}

uint16_t hton16(uint16_t value) { return htons(value); }

uint16_t ntoh16(uint16_t value) { return ntohs(value); }
