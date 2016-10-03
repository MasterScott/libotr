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

#include <stdlib.h>

#include <gcrypt.h>

#include "chat_types.h"
#include "chat_sign.h"
#include "chat_message.h"
#include "chat_participant.h"
#include "chat_protocol.h"
#include "list.h"

int chat_attest_assoctable_hash(OtrlList *partList, unsigned char **hash)
{
	OtrlListNode *cur;
	unsigned char *buf, *key;
	gcry_md_hd_t md;
	gcry_error_t g_err;
    int error;
	size_t len;

	g_err = gcry_md_open(&md, GCRY_MD_SHA512, 0);
	if(g_err) { goto error; }

	for(cur=partList->head; cur!=NULL; cur=cur->next) {
		ChatParticipant *part = cur->payload;
		if(part->sign_key == NULL) { goto error_with_md; }

		error = chat_sign_serialize_pubkey(part->sign_key, &key, &len);
        if(error) { goto error_with_md; }

		gcry_md_write(md, key, len);
		free(key);
	}

	buf = malloc(CHAT_ATTEST_ASSOCTABLE_HASH_LENGTH * sizeof *buf);
	if(buf == NULL) { goto error_with_md; }

	memcpy(buf, gcry_md_read(md, GCRY_MD_SHA512), CHAT_ATTEST_ASSOCTABLE_HASH_LENGTH);
	gcry_md_close(md);

	*hash = buf;
	return 0;

error_with_md:
	gcry_md_close(md);
error:
	return 1;
}

int chat_attest_verify_sid(OtrlChatContext *ctx, unsigned char *sid)
{
	int res, eq;

	eq = memcmp(ctx->sid, sid, CHAT_OFFER_SID_LENGTH);
	res = (eq==0) ? 1 : 0;

	return res;
}

int chat_attest_verify_assoctable_hash(OtrlChatContext *ctx, unsigned char *hash, int *result)
{
	int err, res, eq;
	unsigned char *ourhash;

	err = chat_attest_assoctable_hash(ctx->participants_list, &ourhash);
	if(err) { goto error; }

	eq = memcmp(ourhash, hash, CHAT_ATTEST_ASSOCTABLE_HASH_LENGTH);

	res = (eq==0) ? 1 : 0;

	free(ourhash);

	*result = res;
	return 0;

error:
	return 1;
}

int chat_attest_is_ready(ChatAttestInfo *info)
{
	return (info->checked_count == info->size) ? 1 : 0;
}

int chat_attest_verify(OtrlChatContext *ctx, unsigned char *sid, unsigned char *assoctable_hash, unsigned int part_pos, int *result)
{
	int err, res;

	if(part_pos >= ctx->attest_info->size) { goto error; }

	if(ctx->attest_info->checked[part_pos]) {
		ctx->attest_info->checked[part_pos] = 0;
		ctx->attest_info->checked_count--;
	}

	res = chat_attest_verify_sid(ctx, sid);
	if(res) {
		err = chat_attest_verify_assoctable_hash(ctx, assoctable_hash, &res);
		if(err) { goto error; }
	}

	if(res) {
		ctx->attest_info->checked[part_pos] = 1;
		ctx->attest_info->checked_count++;
	}

	*result = res;
	return 0;

error:
	return 1;

}

void chat_attest_info_free(ChatAttestInfo *info)
{
	if(info) {
		free(info->checked);
	}
	free(info);
}

int chat_attest_info_init(OtrlChatContext *ctx)
{
	ChatAttestInfo *info;

	info = malloc(sizeof *(ctx->attest_info));
	if(!info) { goto error; }

	info->size = otrl_list_length(ctx->participants_list);
	info->checked_count = 0;

	info->checked = calloc(info->size, sizeof *(info->checked));
	if(!info->checked) { goto error_with_info; }

	/*for(unsigned int i=0; i<info->size; i++) {
		info->checked[i] = 0;
	}*/

	info->state = CHAT_ATTESTSTATE_AWAITING;

	chat_attest_info_free(ctx->attest_info);
	ctx->attest_info = NULL;

	ctx->attest_info = info;

	return 0;

error_with_info:
	free(info);
error:
	return 1;
}

int chat_attest_create_our_message(OtrlChatContext *ctx, unsigned int our_pos , ChatMessage **msgToSend)
{
	int err;
	unsigned char *assoctable_hash;
	ChatMessage *msg;

	fprintf(stderr, "libotr-mpOTR: chat_attest_create_our_message: start\n");

	err = chat_attest_assoctable_hash(ctx->participants_list, &assoctable_hash);
	if(err) { goto error; }

	msg = chat_message_attest_create(ctx, ctx->sid, assoctable_hash);
	if(!msg) { goto error_with_assoctable_hash; }

	free(assoctable_hash);

	fprintf(stderr, "libotr-mpOTR: chat_attest_create_our_message: end\n");

	*msgToSend = msg;
	return 0;

error_with_assoctable_hash:
	free(assoctable_hash);
error:
	return 1;
}

int chat_attest_init(OtrlChatContext *ctx, ChatMessage **msgToSend)
{
	int err;
	unsigned int our_pos;
	ChatMessage *ourMsg = NULL;

	fprintf(stderr, "libotr-mpOTR: chat_attest_init: start\n");

	if(!ctx->attest_info) {
		err = chat_attest_info_init(ctx);
		if(err) { goto error; }
	}

	err = chat_participant_get_position(ctx->participants_list, ctx->accountname, &our_pos);
	if(err) { goto error; }

	if(!ctx->attest_info->checked[our_pos]) {
		err = chat_attest_create_our_message(ctx, our_pos, &ourMsg);
		if(err) { goto error; }
		ctx->attest_info->checked[our_pos] = 1;
		ctx->attest_info->checked_count++;
	}

	ctx->attest_info->state = CHAT_ATTESTSTATE_AWAITING;

	fprintf(stderr, "libotr-mpOTR: chat_attest_init: end\n");

	*msgToSend = ourMsg;
	return 0;

error:
	return 1;

}

int chat_attest_handle_message(OtrlChatContext *ctx, const ChatMessage *msg, ChatMessage **msgToSend)
{
	unsigned int our_pos, their_pos;
	int res, err;
	ChatMessagePayloadAttest *payload;
	ChatMessage *ourMsg = NULL;

	fprintf(stderr, "libotr-mpOTR: chat_attest_handle_message: start\n");

	if(!ctx->attest_info) {
		err = chat_attest_info_init(ctx);
		if(err) { goto error; }
	}

	if(msg->msgType != CHAT_MSGTYPE_ATTEST) { goto error; }
	if(ctx->attest_info->state != CHAT_ATTESTSTATE_AWAITING) { goto error; }

	payload = msg->payload;

	err = chat_participant_get_position(ctx->participants_list, msg->senderName, &their_pos);
	if(err) { goto error; }

	err = chat_attest_verify(ctx, payload->sid, payload->assoctable_hash, their_pos, &res);
	if(err) { goto error; }

	if(res == 0) {
		fprintf(stderr, "libotr-mpOTR: chat_attest_handle_message: attest verification failed for participant #: %u\n", their_pos);
		//TODO call protocol reset in chat_protocol.c
		//chat_protocol_reset(ctx);
		goto error;
	} else {

		// Create our attest message if we haven't already sent one
		err = chat_participant_get_position(ctx->participants_list, ctx->accountname, &our_pos);
		if(err) { goto error; }

		if(!ctx->attest_info->checked[our_pos]) {
			err = chat_attest_create_our_message(ctx, our_pos, &ourMsg);
			if(err) { goto error; }

			ctx->attest_info->checked[our_pos] = 1;
			ctx->attest_info->checked_count++;
		}

		if(chat_attest_is_ready(ctx->attest_info)) {
			ctx->attest_info->state = CHAT_ATTESTSTATE_FINISHED;
			ctx->msg_state = OTRL_MSGSTATE_ENCRYPTED;
		}
	}

	fprintf(stderr, "libotr-mpOTR: chat_attest_handle_message: end\n");

	*msgToSend = ourMsg;
	return 0;

error:
	return 1;
}


int chat_attest_is_my_message(ChatMessage *msg)
{
	ChatMessageType msg_type = msg->msgType;

	switch(msg_type) {
		case CHAT_MSGTYPE_ATTEST:
			return 1;
		default:
			return 0;
	}
}
