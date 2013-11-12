//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//Copyright (C) 2013 LunarG, Inc.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//
/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder. 

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// For recording and playing back the stream of tokens in a macro definition.
//

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define snprintf sprintf_s
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "PpContext.h"
#include "PpTokens.h"

namespace glslang {

TPpContext::TokenBlock* TPpContext::lNewBlock(TokenStream *fTok, MemoryPool *pool)
{
    TokenBlock *lBlock;

    if (!pool)
        lBlock = (TokenBlock *) malloc(sizeof(TokenBlock) + 256);
    else
        lBlock = (TokenBlock *) mem_Alloc(pool, sizeof(TokenBlock) + 256);
    lBlock->count = 0;
    lBlock->current = 0;
    lBlock->data = (unsigned char *) lBlock + sizeof(TokenBlock);
    lBlock->max = 256;
    lBlock->next = NULL;
    if (fTok->head) {
        fTok->current->next = lBlock;
    } else {
        fTok->head = lBlock;
    }
    fTok->current = lBlock;

    return lBlock;
}

void TPpContext::lAddByte(TokenStream *fTok, unsigned char fVal)
{
    TokenBlock *lBlock;
    lBlock = fTok->current;
    if (lBlock->count >= lBlock->max)
        lBlock = lNewBlock(fTok, 0);
    lBlock->data[lBlock->count++] = fVal;
}

/*
* Get the next byte from a stream.
*/
int TPpContext::lReadByte(TokenStream *pTok)
{
    TokenBlock *lBlock;
    int lval = -1;

    lBlock = pTok->current;
    if (lBlock) {
        if (lBlock->current >= lBlock->count) {
            lBlock = lBlock->next;
            if (lBlock)
                lBlock->current = 0;
            pTok->current = lBlock;
        }
        if (lBlock)
            lval = lBlock->data[lBlock->current++];
    }
    return lval;
}

//
// Make a token stream (used for reprocessing macros).
//
TPpContext::TokenStream* TPpContext::NewTokenStream(MemoryPool *pool)
{
    TokenStream *pTok;

    if (!pool)
        pTok = (TokenStream *) malloc(sizeof(TokenStream));
    else
        pTok = (TokenStream*)mem_Alloc(pool, sizeof(TokenStream));
    pTok->next = NULL;
    pTok->head = NULL;
    pTok->current = NULL;
    lNewBlock(pTok, pool);
    return pTok;
}

void TPpContext::DeleteTokenStream(TokenStream *pTok)
{
    TokenBlock *pBlock, *nBlock;

    if (pTok) {
        pBlock = pTok->head;
        while (pBlock) {
            nBlock = pBlock->next;
            free(pBlock);
            pBlock = nBlock;
        }
        free(pTok);
    }
}

/*
* Add a token to the end of a list for later playback.
*/
void TPpContext::RecordToken(TokenStream *pTok, int token, TPpToken* ppToken)
{
    const char *s;
    char *str = NULL;

    if (token > 256)
        lAddByte(pTok, (unsigned char)((token & 0x7f) + 0x80));
    else
        lAddByte(pTok, (unsigned char)(token & 0x7f));
    switch (token) {
    case CPP_IDENTIFIER:
    case CPP_STRCONSTANT:
        s = GetAtomString(ppToken->atom);
        while (*s)
            lAddByte(pTok, (unsigned char) *s++);
        lAddByte(pTok, 0);
        break;
    case CPP_INTCONSTANT:
    case CPP_UINTCONSTANT:
    case CPP_FLOATCONSTANT:
    case CPP_DOUBLECONSTANT:
        str = ppToken->name;
        while (*str){
            lAddByte(pTok, (unsigned char) *str);
            str++;
        }
        lAddByte(pTok, 0);
        break;
    case '(':
        lAddByte(pTok, (unsigned char)(ppToken->ival ? 1 : 0));
        break;
    default:
        break;
    }
}

/*
* Reset a token stream in preperation for reading.
*/
void TPpContext::RewindTokenStream(TokenStream *pTok)
{
    if (pTok->head) {
        pTok->current = pTok->head;
        pTok->current->current = 0;
    }
}

/*
* Read the next token from a token stream (not the source stream, but stream used to hold a tokenized macro).
*/
int TPpContext::ReadToken(TokenStream *pTok, TPpToken *ppToken)
{
    char tokenText[TPpToken::maxTokenLength + 1];
    int ltoken, len;
    char ch;

    ltoken = lReadByte(pTok);
    ppToken->loc = parseContext.getCurrentLoc();
    if (ltoken >= 0) {
        if (ltoken > 127)
            ltoken += 128;
        switch (ltoken) {
        case '(':
            ppToken->ival = lReadByte(pTok);
            break;
        case CPP_STRCONSTANT:
        case CPP_IDENTIFIER:
        case CPP_FLOATCONSTANT:
        case CPP_DOUBLECONSTANT:
        case CPP_INTCONSTANT:
        case CPP_UINTCONSTANT:
            len = 0;
            ch = lReadByte(pTok);
            while (ch != 0) {
                if (len < TPpToken::maxTokenLength) {
                    tokenText[len] = ch;
                    len++;
                    ch = lReadByte(pTok);
                } else {
                    parseContext.error(ppToken->loc, "token too long", "", "");
                    break;
                }
            }
            tokenText[len] = 0;

            switch (ltoken) {
            case CPP_IDENTIFIER:
            case CPP_STRCONSTANT:
                ppToken->atom = LookUpAddString(tokenText);
                break;
            case CPP_FLOATCONSTANT:
            case CPP_DOUBLECONSTANT:
                strcpy(ppToken->name, tokenText);
                ppToken->dval = atof(ppToken->name);
                break;
            case CPP_INTCONSTANT:
            case CPP_UINTCONSTANT:
                strcpy(ppToken->name, tokenText);
                if (len > 0 && tokenText[0] == '0') {
                    if (len > 1 && tokenText[1] == 'x' || tokenText[1] == 'X')
                        ppToken->ival = strtol(ppToken->name, 0, 16);
                    else
                        ppToken->ival = strtol(ppToken->name, 0, 8);
                } else
                    ppToken->ival = atoi(ppToken->name);
                break;
            }
        }
        return ltoken;
    }
    return EOF;
}

int TPpContext::scan_token(TPpContext* pp, TokenInputSrc *in, TPpToken * ppToken)
{
    int token = pp->ReadToken(in->tokens, ppToken);
    int (*final)(TPpContext *);
    if (token > 0)
        return token;
    
    pp->currentInput = in->base.prev;
    final = in->final;
    free(in);
    if (final && !final(pp))
        return -1;

    return pp->currentInput->scan(pp, pp->currentInput, ppToken);
}

int TPpContext::ReadFromTokenStream(TokenStream *ts, int name, int (*final)(TPpContext *))
{
    TokenInputSrc *in = (TokenInputSrc *) malloc(sizeof(TokenInputSrc));
    memset(in, 0, sizeof(TokenInputSrc));
    in->base.prev = currentInput;
    in->base.scan = (int (*)(TPpContext*, InputSrc*, TPpToken*))scan_token;
    in->tokens = ts;
    in->final = final;
    RewindTokenStream(ts);
    currentInput = &in->base;

    return 1;
}

int TPpContext::reget_token(TPpContext* pp, UngotToken *t, TPpToken * ppToken)
{
    int token = t->token;
    *ppToken = t->lval;
    pp->currentInput = t->base.prev;
    free(t);
    return token;
}

void TPpContext::UngetToken(int token, TPpToken* ppToken)
{
    UngotToken *t = (UngotToken *) malloc(sizeof(UngotToken));
    memset(t, 0, sizeof(UngotToken));
    t->token = token;
    t->lval = *ppToken;
    t->base.scan = (int(*)(TPpContext*, struct InputSrc *, TPpToken *))reget_token;
    t->base.prev = currentInput;
    currentInput = &t->base;
}

} // end namespace glslang