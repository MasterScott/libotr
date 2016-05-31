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

#include "chat_sign.h"

#include <gcrypt.h>

#define SIGN_HASH_SIZE 64

void chat_sign_print_pubkey(SignKey *key)
{
	gcry_sexp_dump(key->pub_key);
}

SignKey * chat_sign_genkey()
{
    gcry_error_t err;
    gcry_sexp_t key, params;
    SignKey *sign_key;
    static const char *parmstr = "(genkey (ecc (curve Ed25519 (flags eddsa))))";

    err = gcry_sexp_new(&params, parmstr, strlen(parmstr), 0);
    if(err) {
        return NULL;
    }

    err = gcry_pk_genkey(&key, params);
    gcry_sexp_release(params);
    if(err) {
        return NULL;
    }


    sign_key = malloc(sizeof(*sign_key));
    if(!key) {
        gcry_sexp_release(key);
        return NULL;
    }

    sign_key->priv_key = gcry_sexp_find_token(key, "private-key", 0);
    sign_key->pub_key  = gcry_sexp_find_token(key, "public-key", 0);
    //chat_sign_print_pubkey(sign_key);

    return sign_key;
}

int chat_sign_get_data_hash(const unsigned char *data, size_t datalen, unsigned char *hash)
{
    gcry_error_t err;
    gcry_md_hd_t md;

    fprintf(stderr,"libotr-mpOTR: chat_sign_get_data_hash: start\n");

    err = gcry_md_open(&md, GCRY_MD_SHA512, 0);
    if(err){
        return 1;
    }

    fprintf(stderr,"libotr-mpOTR: chat_sign_get_data_hash: after open\n");
    gcry_md_write(md, data, datalen);

    fprintf(stderr,"libotr-mpOTR: chat_sign_get_data_hash: after write\n");

    memcpy(hash, gcry_md_read(md, GCRY_MD_SHA512), SIGN_HASH_SIZE);
    fprintf(stderr,"libotr-mpOTR: chat_sign_get_data_hash: after memcpy\n");
    //hash[SIGN_HASH_SIZE] = '\0';
    gcry_md_close(md);

    fprintf(stderr,"libotr-mpOTR: chat_sign_get_data_hash: end\n");
    return 0;
}

Signature * chat_sign_sign(SignKey *key, const unsigned char *data, size_t datalen)
{
    unsigned char hash[SIGN_HASH_SIZE];
    static char *datastr = "(data (flags eddsa) (hash-algo sha512) (value %m))";
    gcry_mpi_t datampi;
    gcry_sexp_t datas, sigs, temp, r, s;
    const char *token = NULL;
    size_t tokenlen;
    Signature *signature = NULL;
    gcry_error_t err;

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: start\n");

    signature = malloc(sizeof(*signature));
    if(!signature){
        return NULL;
    }

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after signature allocation\n");
    if(chat_sign_get_data_hash(data, datalen, hash)) {
        free(signature);
        return NULL;
    }
    gcry_mpi_scan(&datampi, GCRYMPI_FMT_USG, hash, SIGN_HASH_SIZE, NULL);

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: hash is:");
    for(unsigned int i = 0; i < SIGN_HASH_SIZE ; i++)
        fprintf(stderr,"%02X",hash[i]);
    fprintf(stderr,"\n");

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after get_data_hash\n");

    err = gcry_sexp_build(&datas, NULL, datastr, datampi);
    if(err) {
        free(signature);
        return NULL;
    }
    gcry_sexp_dump(datas);
    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after sexp build\n");

    gcry_sexp_dump(key->pub_key);
    err = gcry_pk_sign(&sigs, datas, key->priv_key);
    gcry_sexp_release(datas);
    if(err) {
        free(signature);
        return NULL;
    }

    gcry_sexp_dump(sigs);

    temp = gcry_sexp_find_token(sigs, "sig-val", 0);
    gcry_sexp_release(sigs);

    r = gcry_sexp_find_token(temp, "r", 0);
    s = gcry_sexp_find_token(temp, "s", 0);
    gcry_sexp_release(temp);

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after the tokens are found\n");

    //gcry_sexp_dump(r);
    token = gcry_sexp_nth_data(r, 1, &tokenlen);
    if(!token) {
        gcry_sexp_release(r);
        gcry_sexp_release(s);
        free(signature);
        return NULL;
    }

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after getting r\n");
    signature->r = malloc(tokenlen * sizeof(*signature->r));
    if(!signature->r){
        gcry_sexp_release(r);
        gcry_sexp_release(s);
        free(signature);
        return NULL;
    }
    memcpy(signature->r, token, tokenlen);
    signature->rlen = tokenlen;

    gcry_sexp_release(r);

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after copying r in signature\n");

    //gcry_sexp_dump(s);
    token = gcry_sexp_nth_data(s, 1, &tokenlen);
    if(!token) {
        gcry_sexp_release(s);
        free(signature->r);
        free(signature);
        return NULL;
    }

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: after getting s\n");

    signature->s = malloc(tokenlen * sizeof(*signature->s));
    if(!signature->s) {
        gcry_sexp_release(s);
        free(signature->r);
        free(signature);
        return NULL;
    }
    memcpy(signature->s, token, tokenlen);
    signature->slen = tokenlen;
    gcry_sexp_release(s);

    fprintf(stderr,"libotr-mpOTR: chat_sign_sign: r is:");
    for(unsigned int i = 0; i < signature->rlen; i++)
        fprintf(stderr,"%02X",signature->r[i]);

    fprintf(stderr,"\nlibotr-mpOTR: chat_sign_sign: s is:");
    for(unsigned int i = 0; i < signature->slen; i++)
        fprintf(stderr,"%02X",signature->s[i]);

    fprintf(stderr,"\nlibotr-mpOTR: chat_sign_sign: end\n");
    return signature;
}

int chat_sign_verify(SignKey *key, const unsigned char *data, size_t datalen, Signature *signature)
{

    unsigned char hash[SIGN_HASH_SIZE + 1];
	gcry_mpi_t datampi;
	gcry_sexp_t sigs;
    gcry_sexp_t datas;
    gcry_error_t err;
    static char *datastr = "(data (flags eddsa) (hash-algo sha512) (value %m))";

    fprintf(stderr,"libotr-mpOTR: chat_sign_verify: start\n");

    fprintf(stderr,"libotr-mpOTR: chat_sign_verify: r is:");
    for(unsigned int i = 0; i < signature->rlen; i++)
        fprintf(stderr,"%02X",signature->r[i]);

    fprintf(stderr,"\nlibotr-mpOTR: chat_sign_verify: s is:");
    for(unsigned int i = 0; i < signature->slen; i++)
        fprintf(stderr,"%02X",signature->s[i]);

    fprintf(stderr,"\nlibotr-mpOTR: chat_sign_verify: after get_data_hash\n");

    if (datalen) {
        fprintf(stderr,"libotr-mpOTR: chat_sign_verify: datalen is not zero\n");
        if(chat_sign_get_data_hash(data, datalen, hash)) {
            fprintf(stderr,"libotr-mpOTR: chat_sign_verify: hash errored\n");
            //free(signature);
            return 1;
        }
    	gcry_mpi_scan(&datampi, GCRYMPI_FMT_USG, hash, SIGN_HASH_SIZE, NULL);
    } else {
    	datampi = gcry_mpi_set_ui(NULL, 0);
    }

    fprintf(stderr,"libotr-mpOTR: chat_sign_verify: hash is:");
    for(unsigned int i = 0; i < SIGN_HASH_SIZE ; i++)
        fprintf(stderr,"%02X",hash[i]);
    fprintf(stderr,"\n");

    gcry_sexp_build(&datas, NULL, datastr, datampi);
    gcry_sexp_dump(datas);
    gcry_sexp_build(&sigs, NULL, "(sig-val (eddsa (r %b)(s %b)))", signature->rlen, signature->r, signature->slen, signature->s);
    gcry_sexp_dump(sigs);

    gcry_sexp_dump(key->pub_key);
    err = gcry_pk_verify(sigs, datas, key->pub_key);
    fprintf(stderr, "libotr-mpOTR: chat_sign_verify: erro is: %s\n",gcry_strerror(err));
    gcry_sexp_release(datas);
    gcry_sexp_release(sigs);

    return err;
}

void chat_sign_serialize_pubkey(SignKey *key, unsigned char **serialized, size_t *serlen)
{
    gcry_sexp_t temps, pubvals;

    temps = gcry_sexp_find_token(key->pub_key, "ecc", 0);
    pubvals = gcry_sexp_find_token(temps, "q", 0);

    *serialized = (unsigned char*) gcry_sexp_nth_data(pubvals, 1, serlen);
}

SignKey * chat_sign_parse_pubkey(const unsigned char *serialized, size_t serlen) {
	static char *datastr = "(public-key (ecc (curve Ed25519) (flags eddsa) (q  %b)))";
	SignKey *key;

	key = malloc(sizeof(*key));
	if(!key) {
		return NULL;
	}

	gcry_sexp_build(&key->pub_key, NULL, datastr, serlen, serialized);
	key->priv_key = NULL;

	return key;
}

void chat_sign_destroy_key(SignKey *key)
{
	gcry_sexp_release(key->priv_key);
	gcry_sexp_release(key->pub_key);
	free(key);
}

void chat_sign_destroy_signature(Signature *sign)
{
    fprintf(stderr, "libotr-mpOTR: chat_sign_destroy_signature: start\n");
	free(sign->r);
    fprintf(stderr, "libotr-mpOTR: chat_sign_destroy_signature: after free r\n");
	free(sign->s);
    fprintf(stderr, "libotr-mpOTR: chat_sign_destroy_signature: after free s\n");
	free(sign);
    fprintf(stderr, "libotr-mpOTR: chat_sign_destroy_signature: end\n");
}


unsigned int chat_sign_signature_get_length(Signature *sig)
{
	if(!sig || !sig->s || !sig->r) {
		return 0;
	}
	return sizeof 4*sizeof(char) + sig->rlen*sizeof(*sig->r) + 4*sizeof(char) + sig->slen*sizeof(*sig->s);
}

int chat_sign_signature_serialize(Signature *sig, unsigned char **buf, size_t *len)
{
	size_t base = 0;
	if(!sig || !sig->s || !sig->r) {
		return 1;
	}

    *len = chat_sign_signature_get_length(sig);
	*buf = malloc(*len * sizeof(**buf));
	if(!*buf) {
		return 1;
	}
    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_serialize: *buf = %p\n", *buf);

	memcpy(*buf + base, &sig->rlen, 4*sizeof(char));
	base += 4*sizeof(char);
	memcpy(*buf + base, sig->r, sig->rlen);
	base += sig->rlen;
	memcpy(*buf + base, &sig->slen, 4*sizeof(char));
	base += 4*sizeof(char);
	memcpy(*buf + base, sig->s, sig->slen);

	return 0;
}

int chat_sign_signature_parse(unsigned char *buf, Signature **sig)
{
	size_t base = 0;

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: start\n");

	if(!buf) {
		return 1;
	}

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after buf check\n");

	*sig = malloc(sizeof(**sig));
	if(!*sig) {
		return 1;
	}

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after sig alloc\n");

	memcpy(&(*sig)->rlen, buf + base, 4*sizeof(char));
    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: rlen %u\n", (*sig)->rlen);
	(*sig)->r = malloc((*sig)->rlen * sizeof(*(*sig)->r));
	if(!(*sig)->r){
		free(*sig);
		return 1;
	}
	base += 4*sizeof(char);

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after memcpy\n");

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: rlen %u\n", (*sig)->rlen * sizeof(*(*sig)->r));
	memcpy((*sig)->r, buf + base, (*sig)->rlen * sizeof(*(*sig)->r));
    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after memcpy (1) (base is: %u, rlen is: %u)\n", base, (*sig)->rlen);
	base += (*sig)->rlen;

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after memcpy (2) (base is: %d)\n", base);

    memcpy(&(*sig)->slen, buf + base, 4*sizeof(char));
    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: slen %d\n", (*sig)->slen);
	(*sig)->s = malloc((*sig)->slen * sizeof(*(*sig)->s));
	if(!(*sig)->s){
		free((*sig)->r);
		free(*sig);
		return 1;
	}
	base += 4*sizeof(char);

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after memcpy\n");

	memcpy((*sig)->s, buf + base, (*sig)->slen * sizeof(char));

    fprintf(stderr, "libotr-mpOTR: chat_sign_signature_parse: after memcpy\n");
	return 0;
}