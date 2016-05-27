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

#include "chat_types.h"
#include "message.h"

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

int chat_message_is_otr(const char * message);

void chat_message_free(OtrlChatMessage * msg);

int chat_message_type_should_be_signed(OtrlMessageType type);

unsigned char * chat_message_serialize(OtrlChatMessage *msg, size_t *length);

int chat_message_parse_type(const char *message, const size_t messagelen, OtrlChatMessageType *type);

OtrlChatMessage * chat_message_parse(const unsigned char *message, const size_t messagelen, const char *accountname);

OtrlChatMessage * chat_message_offer_create(OtrlChatContext *ctx, unsigned char *sid_contribution, unsigned int position);

OtrlChatMessage * chat_message_dake_handshake_create(OtrlChatContext *ctx, DAKE_handshake_message_data *data);

OtrlChatMessage * chat_message_dake_confirm_create(OtrlChatContext *ctx, unsigned int recipient, DAKE_confirm_message_data *data);

OtrlChatMessage * chat_message_dake_key_create(OtrlChatContext *ctx, unsigned int recipient, DAKE_key_message_data *data);

OtrlChatMessage * chat_message_gka_upflow_create(OtrlChatContext *ctx, OtrlList *interKeys, unsigned int recipient);

OtrlChatMessage * chat_message_gka_downflow_create(OtrlChatContext *ctx, OtrlList *interKeys);

OtrlChatMessage * chat_message_data_create(OtrlChatContext *ctx, unsigned char *ctr, size_t datalen, unsigned char *ciphertext);

int chat_message_send(const OtrlMessageAppOps *ops, OtrlChatContext *ctx, OtrlChatMessage *msg);

#endif /* CHAT_MESSAGE_H_ */
