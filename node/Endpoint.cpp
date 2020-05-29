/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#include "Endpoint.hpp"
#include "Utils.hpp"

namespace ZeroTier {

void Endpoint::toString(char s[ZT_ENDPOINT_STRING_SIZE_MAX]) const noexcept
{
	static const char *const s_endpointTypeChars = ZT_ENDPOINT_TYPE_CHAR_INDEX;

	static_assert(ZT_ENDPOINT_STRING_SIZE_MAX > (ZT_INETADDRESS_STRING_SIZE_MAX + 4), "overflow");
	static_assert(ZT_ENDPOINT_STRING_SIZE_MAX > (ZT_FINGERPRINT_STRING_SIZE_MAX + 4), "overflow");

	switch (this->type) {
		default:
			s[0] = s_endpointTypeChars[ZT_ENDPOINT_TYPE_NIL];
			s[1] = 0;
			break;
		case ZT_ENDPOINT_TYPE_ZEROTIER:
			s[0] = s_endpointTypeChars[ZT_ENDPOINT_TYPE_ZEROTIER];
			s[1] = '/';
			zt().toString(s + 2);
			break;
		case ZT_ENDPOINT_TYPE_ETHERNET:
		case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
		case ZT_ENDPOINT_TYPE_BLUETOOTH:
			s[0] = s_endpointTypeChars[this->type];
			s[1] = '/';
			eth().toString(s + 2);
			break;
		case ZT_ENDPOINT_TYPE_IP:
		case ZT_ENDPOINT_TYPE_IP_UDP:
		case ZT_ENDPOINT_TYPE_IP_TCP:
		case ZT_ENDPOINT_TYPE_IP_HTTP2:
			s[0] = s_endpointTypeChars[this->type];
			s[1] = '/';
			ip().toString(s + 2);
			break;
	}
}

bool Endpoint::fromString(const char *s) noexcept
{
	memoryZero(this);
	if ((!s) || (!*s))
		return true;

	const char *start = strchr(s, '/');
	if (start) {
		char tmp[16];
		for (unsigned int i = 0;i < 15;++i) {
			if ((tmp[i] = s[i]) == 0)
				break;
		}
		tmp[15] = 0;
		this->type = (ZT_EndpointType)Utils::strToUInt(tmp);

		++start;
		Fingerprint tmpfp;
		MAC tmpmac;
		switch (this->type) {
			case ZT_ENDPOINT_TYPE_NIL:
				break;
			case ZT_ENDPOINT_TYPE_ZEROTIER:
				if (!tmpfp.fromString(start))
					return false;
				this->value.fp = tmpfp;
				break;
			case ZT_ENDPOINT_TYPE_ETHERNET:
			case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
			case ZT_ENDPOINT_TYPE_BLUETOOTH:
				tmpmac.fromString(start);
				this->value.mac = tmpmac.toInt();
				break;
			case ZT_ENDPOINT_TYPE_IP:
			case ZT_ENDPOINT_TYPE_IP_UDP:
			case ZT_ENDPOINT_TYPE_IP_TCP:
			case ZT_ENDPOINT_TYPE_IP_HTTP2:
				if (!asInetAddress(this->value.ss).fromString(start))
					return false;
			default:
				this->type = ZT_ENDPOINT_TYPE_NIL;
				return false;
		}
	} else {
		if (Utils::strToUInt(s) != (unsigned int)ZT_ENDPOINT_TYPE_NIL)
			return false;
	}

	return true;
}

int Endpoint::marshal(uint8_t data[ZT_ENDPOINT_MARSHAL_SIZE_MAX]) const noexcept
{
	switch (this->type) {
		//case ZT_ENDPOINT_TYPE_NIL:
		default:
			// NIL endpoints get serialized like NIL InetAddress instances.
			data[0] = ZT_ENDPOINT_TYPE_NIL;
			return 1;

		case ZT_ENDPOINT_TYPE_ZEROTIER:
			data[0] = 16 + ZT_ENDPOINT_TYPE_ZEROTIER;
			Address(this->value.fp.address).copyTo(data + 1);
			Utils::copy<ZT_FINGERPRINT_HASH_SIZE>(data + 1 + ZT_ADDRESS_LENGTH, this->value.fp.hash);
			return 1 + ZT_ADDRESS_LENGTH + ZT_FINGERPRINT_HASH_SIZE;

		case ZT_ENDPOINT_TYPE_ETHERNET:
		case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
		case ZT_ENDPOINT_TYPE_BLUETOOTH:
			data[0] = 16 + (uint8_t)this->type;
			MAC(this->value.mac).copyTo(data + 1);
			return 7;

		case ZT_ENDPOINT_TYPE_IP_UDP:
			// Default UDP mode gets serialized to look exactly like an InetAddress.
			return asInetAddress(this->value.ss).marshal(data);

		case ZT_ENDPOINT_TYPE_IP:
		case ZT_ENDPOINT_TYPE_IP_TCP:
		case ZT_ENDPOINT_TYPE_IP_HTTP2:
			// Other IP types get serialized as new version Endpoint instances with type.
			data[0] = 16 + (uint8_t)this->type;
			return 1 + asInetAddress(this->value.ss).marshal(data + 1);
	}
}

int Endpoint::unmarshal(const uint8_t *restrict data, int len) noexcept
{
	memoryZero(this);
	if (unlikely(len <= 0))
		return -1;

	// Serialized endpoints with type bytes less than 16 are passed through
	// to the unmarshal method of InetAddress and considered UDP endpoints.
	// This allows backward compatibility with old endpoint fields in the
	// protocol that were serialized InetAddress instances.
	if (data[0] < 16) {
		switch (data[0]) {
			case 0:
				return 1;
			case 4:
			case 6:
				this->type = ZT_ENDPOINT_TYPE_IP_UDP;
				return asInetAddress(this->value.ss).unmarshal(data, len);
		}
		return -1;
	}

	switch ((this->type = (ZT_EndpointType)(data[0] - 16))) {
		case ZT_ENDPOINT_TYPE_NIL:
			return 1;

		case ZT_ENDPOINT_TYPE_ZEROTIER:
			if (len >= (1 + ZT_ADDRESS_LENGTH + ZT_FINGERPRINT_HASH_SIZE)) {
				this->value.fp.address = Address(data + 1).toInt();
				Utils::copy<ZT_FINGERPRINT_HASH_SIZE>(this->value.fp.hash, data + 1 + ZT_ADDRESS_LENGTH);
				return 1 + ZT_ADDRESS_LENGTH + ZT_FINGERPRINT_HASH_SIZE;
			}
			return -1;

		case ZT_ENDPOINT_TYPE_ETHERNET:
		case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
		case ZT_ENDPOINT_TYPE_BLUETOOTH:
			if (len >= 7) {
				MAC tmp;
				tmp.setTo(data + 1);
				this->value.mac = tmp.toInt();
				return 7;
			}
			return -1;

		case ZT_ENDPOINT_TYPE_IP:
		case ZT_ENDPOINT_TYPE_IP_UDP:
		case ZT_ENDPOINT_TYPE_IP_TCP:
		case ZT_ENDPOINT_TYPE_IP_HTTP2:
			return asInetAddress(this->value.ss).unmarshal(data + 1, len - 1);

		default:
			break;
	}

	// Unrecognized types can still be passed over in a valid stream if they are
	// prefixed by a 16-bit size. This allows forward compatibility with future
	// endpoint types.
	this->type = ZT_ENDPOINT_TYPE_NIL;
	if (len < 3)
		return -1;
	const int unrecLen = 1 + (int) Utils::loadBigEndian<uint16_t>(data + 1);
	return (unrecLen > len) ? -1 : unrecLen;
}

bool Endpoint::operator==(const Endpoint &ep) const noexcept
{
	if (this->type == ep.type) {
		switch(this->type) {
			case ZT_ENDPOINT_TYPE_ZEROTIER:
				return zt() == ep.zt();
			case ZT_ENDPOINT_TYPE_ETHERNET:
			case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
			case ZT_ENDPOINT_TYPE_BLUETOOTH:
				return this->value.mac == ep.value.mac;
			case ZT_ENDPOINT_TYPE_IP:
			case ZT_ENDPOINT_TYPE_IP_UDP:
			case ZT_ENDPOINT_TYPE_IP_TCP:
			case ZT_ENDPOINT_TYPE_IP_HTTP2:
				return ip() == ep.ip();
			default:
				return true;
		}
	}
	return false;
}

bool Endpoint::operator<(const Endpoint &ep) const noexcept
{
	if (this->type == ep.type) {
		switch(this->type) {
			case ZT_ENDPOINT_TYPE_ZEROTIER:
				return zt() < ep.zt();
			case ZT_ENDPOINT_TYPE_ETHERNET:
			case ZT_ENDPOINT_TYPE_WIFI_DIRECT:
			case ZT_ENDPOINT_TYPE_BLUETOOTH:
				return this->value.mac < ep.value.mac;
			case ZT_ENDPOINT_TYPE_IP:
			case ZT_ENDPOINT_TYPE_IP_UDP:
			case ZT_ENDPOINT_TYPE_IP_TCP:
			case ZT_ENDPOINT_TYPE_IP_HTTP2:
				return ip() < ep.ip();
			default:
				return true;
		}
	}
	return (int)this->type < (int)ep.type;
}

} // namespace ZeroTier