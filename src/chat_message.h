/*
 *  Off-the-Record Messaging library
 *  Copyright (C) 2015-2016  Dimitrios Kolotouros <dim.kolotouros@gmail.com>,
 *  						 Konstantinos Andrikopoulos <el11151@mail.ntua.gr>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2.1 of the GNU Lesser General
 *  Public License as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CHAT_MESSAGE_H_
#define CHAT_MESSAGE_H_

#include <stdlib.h>

#include "tlv.h"
#include "proto.h"
#include "message.h"
#include "instag.h"
//#include "chat_token.h"
//#include "chat_context.h"

#include "chat_types.h"
/*
typedef void * MessagePayloadPtr;

typedef struct OtrlChatMessageStruct {
	int16_t protoVersion;
	OtrlMessageType msgType;
	otrl_instag_t senderInsTag;
	otrl_instag_t chatInsTag;
	MessagePayloadPtr payload;
	void (*payload_free)(MessagePayloadPtr);
	void (*payload_serialize)(MessagePayloadPtr);
} OtrlChatMessage;

typedef struct OtrlChatMessagePayloadQueryStruct {
	//TODO this is to change
	unsigned char key[32];
} OtrlChatMessagePayloadQuery;

typedef struct OtrlChatMessagePayloadQueryAckStruct {
	//TODO this is to change
	unsigned char magicnum[4];
} OtrlChatMessagePayloadQueryAck;

typedef struct OtrlChatMessagePayloadDataStruct {
	unsigned char ctr[8];
	int32_t datalen;
	unsigned char *ciphertext;
} OtrlChatMessagePayloadData;
*/

int otrl_chat_message_receiving(OtrlUserState us, const OtrlMessageAppOps *ops,
	void *opdata, const char *accountname, const char *protocol,
	const char *sender, otrl_chat_token_t chat_token, const char *message,
	char **newmessagep,	OtrlTLV **tlvsp);

int otrl_chat_message_sending(OtrlUserState us,
	const OtrlMessageAppOps *ops,
	void *opdata, const char *accountname, const char *protocol,
	const char *message, otrl_chat_token_t chat_token, OtrlTLV *tlvs,
	char **messagep, OtrlFragmentPolicy fragPolicy);

OtrlChatMessage * chat_message_parse(const char *message);

MessagePayloadPtr chat_message_payload_parse(OtrlChatMessage *msg, const unsigned char *message, size_t length);

char * chat_message_serialize(OtrlChatMessage *msg);

MessagePayloadPtr chat_message_payload_query_parse(const unsigned char * message, size_t length);

void chat_message_payload_query_free(MessagePayloadPtr payload);

unsigned char * chat_message_payload_query_serialize(MessagePayloadPtr payload);

MessagePayloadPtr chat_message_payload_query_ack_parse(const unsigned char * message, size_t length);

void chat_message_payload_query_ack_free(MessagePayloadPtr payload);

unsigned char * chat_message_payload_query_ack_serialize(MessagePayloadPtr payload);

MessagePayloadPtr chat_message_payload_data_parse(const unsigned char *message, size_t length);

void chat_message_payload_data_free(MessagePayloadPtr payload);

unsigned char * chat_message_payload_data_serialize(MessagePayloadPtr payload);

size_t chat_message_payload_size(OtrlChatMessage *msg);

OtrlMessageType chat_message_message_type_parse(unsigned char c);

unsigned char chat_message_message_type_serialize(OtrlMessageType msgType);

void chat_message_free(OtrlChatMessage * msg);

int chat_message_is_otr(const char * message);

int chat_message_is_fragment(const char * message);

OtrlChatMessage * chat_message_create(unsigned int proto_version, OtrlMessageType msgType ,otrl_instag_t our_instag);

OtrlChatMessage * chat_message_query_create(int16_t proto_version,
		otrl_instag_t our_instag, const unsigned char *key);

OtrlChatMessage * chat_message_query_ack_create(int16_t protoVersion,
		otrl_instag_t ourInstag);

OtrlChatMessage * chat_message_data_create(int16_t protoVersion,
		otrl_instag_t ourInstag, unsigned char *ctr, size_t datalen, unsigned char *ciphertext);

int chat_message_send(const OtrlMessageAppOps *ops, OtrlChatContext *ctx, OtrlChatMessage *msg);

#endif /* CHAT_MESSAGE_H_ */
