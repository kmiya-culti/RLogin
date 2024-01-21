/*	$OpenBSD: ssh2.h,v 1.9 2003/05/14 00:52:59 markus Exp $	*/

/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * draft-ietf-secsh-architecture-05.txt
 *
 *   Transport layer protocol:
 *
 *     1-19     Transport layer generic (e.g. disconnect, ignore, debug,
 *              etc)
 *     20-29    Algorithm negotiation
 *     30-49    Key exchange method specific (numbers can be reused for
 *              different authentication methods)
 *
 *   User authentication protocol:
 *
 *     50-59    User authentication generic
 *     60-79    User authentication method specific (numbers can be reused
 *              for different authentication methods)
 *
 *   Connection protocol:
 *
 *     80-89    Connection protocol generic
 *     90-127   Channel related messages
 *
 *   Reserved for client protocols:
 *
 *     128-191  Reserved
 *
 *   Local extensions:
 *
 *     192-255  Local extensions
 */

/* ranges */

#define SSH2_MSG_TRANSPORT_MIN				1
#define SSH2_MSG_TRANSPORT_MAX				49
#define SSH2_MSG_USERAUTH_MIN				50
#define SSH2_MSG_USERAUTH_MAX				79
#define SSH2_MSG_USERAUTH_PER_METHOD_MIN	60
#define SSH2_MSG_USERAUTH_PER_METHOD_MAX	SSH2_MSG_USERAUTH_MAX
#define SSH2_MSG_CONNECTION_MIN				80
#define SSH2_MSG_CONNECTION_MAX				127
#define SSH2_MSG_RESERVED_MIN				128
#define SSH2_MSG_RESERVED_MAX				191
#define SSH2_MSG_LOCAL_MIN					192
#define SSH2_MSG_LOCAL_MAX					255
#define SSH2_MSG_MIN						1
#define SSH2_MSG_MAX						255

/* transport layer: generic */

#define SSH2_MSG_DISCONNECT					1
#define SSH2_MSG_IGNORE						2
#define SSH2_MSG_UNIMPLEMENTED				3
#define SSH2_MSG_DEBUG						4
#define SSH2_MSG_SERVICE_REQUEST			5
#define SSH2_MSG_SERVICE_ACCEPT				6
#define SSH2_MSG_EXT_INFO					7	// RFC 8308
#define	SSH2_MSG_NEWCOMPRESS				8	// RFC 8308

/* transport layer: alg negotiation */

#define SSH2_MSG_KEXINIT					20
#define SSH2_MSG_NEWKEYS					21

/* transport layer: kex specific messages, can be reused */

#define SSH2_MSG_KEXDH_INIT					30
#define SSH2_MSG_KEXDH_REPLY				31

/* dh-group-exchange */

#define SSH2_MSG_KEX_DH_GEX_REQUEST_OLD		30
#define SSH2_MSG_KEX_DH_GEX_GROUP			31
#define SSH2_MSG_KEX_DH_GEX_INIT			32
#define SSH2_MSG_KEX_DH_GEX_REPLY			33
#define SSH2_MSG_KEX_DH_GEX_REQUEST			34

/* ecdh */

#define SSH2_MSG_KEX_ECDH_INIT				30
#define SSH2_MSG_KEX_ECDH_REPLY 			31

/* rsa1024-sha1/rsa2048-sha256 */

#define	SSH2_MSG_KEXRSA_PUBKEY				30
#define	SSH2_MSG_KEXRSA_SECRET				31
#define	SSH2_MSG_KEXRSA_DONE				32

/* transport layer: OpenSSH extensions */

#define SSH2_MSG_PING						192
#define SSH2_MSG_PONG						193

/* user authentication: generic */

#define SSH2_MSG_USERAUTH_REQUEST			50
#define SSH2_MSG_USERAUTH_FAILURE			51
#define SSH2_MSG_USERAUTH_SUCCESS			52
#define SSH2_MSG_USERAUTH_BANNER			53

/* user authentication: method specific, can be reused */

#define SSH2_MSG_USERAUTH_PK_OK				60
#define SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ	60
#define SSH2_MSG_USERAUTH_INFO_REQUEST		60
#define SSH2_MSG_USERAUTH_INFO_RESPONSE		61

/* draft-ietf-secsh-gsskeyex-06 */

#define SSH2_MSG_USERAUTH_GSSAPI_RESPONSE			60
#define SSH2_MSG_USERAUTH_GSSAPI_TOKEN				61
#define SSH2_MSG_USERAUTH_GSSAPI_EXCHANGE_COMPLETE	63
#define SSH2_MSG_USERAUTH_GSSAPI_ERROR				64
#define SSH2_MSG_USERAUTH_GSSAPI_ERRTOK 			65
#define SSH2_MSG_USERAUTH_GSSAPI_MIC				66

/* connection protocol: generic */

#define SSH2_MSG_GLOBAL_REQUEST				80
#define SSH2_MSG_REQUEST_SUCCESS			81
#define SSH2_MSG_REQUEST_FAILURE			82

/* channel related messages */

#define SSH2_MSG_CHANNEL_OPEN				90
#define SSH2_MSG_CHANNEL_OPEN_CONFIRMATION	91
#define SSH2_MSG_CHANNEL_OPEN_FAILURE		92
#define SSH2_MSG_CHANNEL_WINDOW_ADJUST		93
#define SSH2_MSG_CHANNEL_DATA				94
#define SSH2_MSG_CHANNEL_EXTENDED_DATA		95
#define SSH2_MSG_CHANNEL_EOF				96
#define SSH2_MSG_CHANNEL_CLOSE				97
#define SSH2_MSG_CHANNEL_REQUEST			98
#define SSH2_MSG_CHANNEL_SUCCESS			99
#define SSH2_MSG_CHANNEL_FAILURE			100

/* disconnect reason code */

#define SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT		1
#define SSH2_DISCONNECT_PROTOCOL_ERROR					2
#define SSH2_DISCONNECT_KEY_EXCHANGE_FAILED				3
#define SSH2_DISCONNECT_HOST_AUTHENTICATION_FAILED		4
#define SSH2_DISCONNECT_RESERVED						4
#define SSH2_DISCONNECT_MAC_ERROR						5
#define SSH2_DISCONNECT_COMPRESSION_ERROR				6
#define SSH2_DISCONNECT_SERVICE_NOT_AVAILABLE			7
#define SSH2_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED	8
#define SSH2_DISCONNECT_HOST_KEY_NOT_VERIFIABLE			9
#define SSH2_DISCONNECT_CONNECTION_LOST					10
#define SSH2_DISCONNECT_BY_APPLICATION					11
#define SSH2_DISCONNECT_TOO_MANY_CONNECTIONS			12
#define SSH2_DISCONNECT_AUTH_CANCELLED_BY_USER			13
#define SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE	14
#define SSH2_DISCONNECT_ILLEGAL_USER_NAME				15

/* misc */

#define SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED	1
#define SSH2_OPEN_CONNECT_FAILED				2
#define SSH2_OPEN_UNKNOWN_CHANNEL_TYPE			3
#define SSH2_OPEN_RESOURCE_SHORTAGE				4

#define SSH2_EXTENDED_DATA_STDERR				1

/* agent */

/* Messages for the authentication agent connection. */
#define SSH_AGENTC_REQUEST_RSA_IDENTITIES			1
#define SSH_AGENT_RSA_IDENTITIES_ANSWER 			2
#define SSH_AGENTC_RSA_CHALLENGE					3
#define SSH_AGENT_RSA_RESPONSE  					4
#define SSH_AGENT_FAILURE							5
#define SSH_AGENT_SUCCESS							6
#define SSH_AGENTC_ADD_RSA_IDENTITY					7
#define SSH_AGENTC_REMOVE_RSA_IDENTITY				8
#define SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES		9

/* private OpenSSH extensions for SSH2 */
#define SSH_AGENTC_REQUEST_IDENTITIES				11
#define SSH_AGENT_IDENTITIES_ANSWER					12
#define SSH_AGENTC_SIGN_REQUEST						13
#define SSH_AGENT_SIGN_RESPONSE						14
#define SSH_AGENTC_ADD_IDENTITY						17
#define SSH_AGENTC_REMOVE_IDENTITY					18
#define SSH_AGENTC_REMOVE_ALL_IDENTITIES			19

#define	SSH_AGENTC_ADD_SMARTCARD_KEY				20
#define	SSH_AGENTC_REMOVE_SMARTCARD_KEY				21

/* lock/unlock the agent */
#define SSH_AGENTC_LOCK 							22
#define SSH_AGENTC_UNLOCK							23

/* add key with constraints */
#define SSH_AGENTC_ADD_RSA_ID_CONSTRAINED			24
#define SSH_AGENTC_ADD_ID_CONSTRAINED  				25
#define SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED	26

#define	SSH_AGENTC_EXTENSION						27
#define	SSH_AGENT_EXTENSION_FAILURE					28

#define SSH_AGENT_CONSTRAIN_LIFETIME				1
#define SSH_AGENT_CONSTRAIN_CONFIRM					2
#define	SSH_AGENT_CONSTRAIN_EXTENSION				3

#define	SSH_AGENT_RSA_SHA2_256						2
#define	SSH_AGENT_RSA_SHA2_512						4

/* additional error code for ssh.com's ssh-agent2 */
#define SSH_COM_AGENT2_FAILURE  					102
#define SSH_AGENT_OLD_SIGNATURE 					0x01
