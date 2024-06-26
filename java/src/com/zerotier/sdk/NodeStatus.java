/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

package com.zerotier.sdk;

import com.zerotier.sdk.util.StringUtils;

/**
 * Current node status
 * <p>
 * Defined in ZeroTierOne.h as ZT_NodeStatus
 */
public class NodeStatus {

    private final long address;

    private final String publicIdentity;

    private final String secretIdentity;

    private final boolean online;

    public NodeStatus(long address, String publicIdentity, String secretIdentity, boolean online) {
        this.address = address;
        this.publicIdentity = publicIdentity;
        this.secretIdentity = secretIdentity;
        this.online = online;
    }

    @Override
    public String toString() {
        return "NodeStatus(" + StringUtils.addressToString(address) + ", " + publicIdentity + ", " + secretIdentity + ", " + online + ")";
    }

	/**
	 * 40-bit ZeroTier address of this node
	 */
    public long getAddress() {
        return address;
    }

	/**
	 * Public identity in string-serialized form (safe to send to others)
	 *
	 * <p>This identity will remain valid as long as the node exists.</p>
	 */
    public String getPublicIdentity() {
        return publicIdentity;
    }

	/**
	 * Full identity including secret key in string-serialized form
	 *
	 * <p>This identity will remain valid as long as the node exists.</p>
	 */
    public String getSecretIdentity() {
        return secretIdentity;
    }

	/**
	 * True if some kind of connectivity appears available
	 */
    public boolean isOnline() {
        return online;
    }
}
