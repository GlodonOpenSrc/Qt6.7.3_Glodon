/**
 * Test the UTF-8 decoding routines
 *
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#define XML_DEPRECATED

#include <stdio.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

int lastError;

static void errorHandler(void *unused, const xmlError *err) {
    if ((unused == NULL) && (err != NULL) && (lastError == 0)) {
        lastError = err->code;
    }
}

char document1[100] = "<doc>XXXX</doc>";
char document2[100] = "<doc foo='XXXX'/>";

static int testDocumentRangeByte1(xmlParserCtxtPtr ctxt, char *document,
                  int len,  char *data, int forbid1, int forbid2) {
    int i;
    xmlDocPtr res;

    for (i = 0;i <= 0xFF;i++) {
	lastError = 0;
	xmlCtxtReset(ctxt);

        data[0] = (char) i;

	res = xmlReadMemory(document, len, "test", NULL, 0);

	if ((i == forbid1) || (i == forbid2)) {
	    if ((lastError == 0) || (res != NULL)) {
	        fprintf(stderr,
		    "Failed to detect invalid char for Byte 0x%02X: %c\n",
		        i, i);
		return(1);
	    }
	}

	else if ((i == '<') || (i == '&')) {
	    if ((lastError == 0) || (res != NULL)) {
	        fprintf(stderr,
		    "Failed to detect illegal char %c for Byte 0x%02X\n", i, i);
		return(1);
	    }
	}
	else if (((i < 0x20) || (i >= 0x80)) &&
	    (i != 0x9) && (i != 0xA) && (i != 0xD)) {
	    if ((lastError != XML_ERR_INVALID_CHAR) && (res != NULL)) {
	        fprintf(stderr,
		    "Failed to detect invalid char for Byte 0x%02X\n", i);
		return(1);
	    }
	}
	else if (res == NULL) {
	    fprintf(stderr,
		"Failed to parse valid char for Byte 0x%02X : %c\n", i, i);
		return(1);
	}
	if (res != NULL)
	    xmlFreeDoc(res);
    }
    return(0);
}

static int testDocumentRangeByte2(xmlParserCtxtPtr ctxt, char *document,
                  int len,  char *data) {
    int i, j;
    xmlDocPtr res;

    for (i = 0x80;i <= 0xFF;i++) {
    for (j = 0;j <= 0xFF;j++) {
	lastError = 0;
	xmlCtxtReset(ctxt);

        data[0] = (char) i;
        data[1] = (char) j;

	res = xmlReadMemory(document, len, "test", NULL, 0);

	/* if first bit of first char is set, then second bit must too */
	if ((i & 0x80) && ((i & 0x40) == 0)) {
	    if ((lastError == 0) || (res != NULL)) {
		fprintf(stderr,
		"Failed to detect invalid char for Bytes 0x%02X 0x%02X\n",
			i, j);
		return(1);
	    }
	}

	/*
	 * if first bit of first char is set, then second char first
	 * bits must be 10
	 */
	else if ((i & 0x80) && ((j & 0xC0) != 0x80)) {
	    if ((lastError == 0) || (res != NULL)) {
		fprintf(stderr,
	    "Failed to detect invalid char for Bytes 0x%02X 0x%02X\n",
			i, j);
		return(1);
	    }
	}

	/*
	 * if using a 2 byte encoding then the value must be greater
	 * than 0x80, i.e. one of bits 5 to 1 of i must be set
	 */
	else if ((i & 0x80) && ((i & 0x1E) == 0)) {
	    if ((lastError == 0) || (res != NULL)) {
		fprintf(stderr,
	    "Failed to detect invalid char for Bytes 0x%02X 0x%02X\n",
			i, j);
		return(1);
	    }
	}

	/*
	 * if third bit of first char is set, then the sequence would need
	 * at least 3 bytes, but we give only 2 !
	 */
	else if ((i & 0xE0) == 0xE0) {
	    if ((lastError == 0) || (res != NULL)) {
		fprintf(stderr,
	    "Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x00\n",
			i, j);
		return(1);
	    }
	}

	/*
	 * We should see no error in remaining cases
	 */
	else if ((lastError != 0) || (res == NULL)) {
	    fprintf(stderr,
		"Failed to parse document for Bytes 0x%02X 0x%02X\n", i, j);
	    return(1);
	}
	if (res != NULL)
	    xmlFreeDoc(res);
    }
    }
    return(0);
}

/**
 * testDocumentRanges:
 *
 * Test the correct UTF8 character parsing in context of XML documents
 * Those are in-context injection tests checking the parser behaviour on
 * edge case values at different point in content, beginning and end of
 * CDATA in text or in attribute values.
 */

static int testDocumentRanges(void) {
    xmlParserCtxtPtr ctxt;
    char *data;
    int test_ret = 0;

    /*
     * Set up a parsing context using the first document as
     * the current input source.
     */
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
	return(1);
    }

    printf("testing 1 byte char in document: 1");
    fflush(stdout);
    data = &document1[5];
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 1 byte injection at beginning of area */
    test_ret += testDocumentRangeByte1(ctxt, &document1[0], strlen(document1),
                           data, -1, -1);
    printf(" 2");
    fflush(stdout);
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 1 byte injection at end of area */
    test_ret += testDocumentRangeByte1(ctxt, &document1[0], strlen(document1),
                           data + 3, -1, -1);

    printf(" 3");
    fflush(stdout);
    data = &document2[10];
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 1 byte injection at beginning of area */
    test_ret += testDocumentRangeByte1(ctxt, &document2[0], strlen(document2),
                           data, '\'', -1);
    printf(" 4");
    fflush(stdout);
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 1 byte injection at end of area */
    test_ret += testDocumentRangeByte1(ctxt, &document2[0], strlen(document2),
                           data + 3, '\'', -1);
    printf(" done\n");

    printf("testing 2 byte char in document: 1");
    fflush(stdout);
    data = &document1[5];
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 2 byte injection at beginning of area */
    test_ret += testDocumentRangeByte2(ctxt, &document1[0], strlen(document1),
                           data);
    printf(" 2");
    fflush(stdout);
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 2 byte injection at end of area */
    test_ret += testDocumentRangeByte2(ctxt, &document1[0], strlen(document1),
                           data + 2);

    printf(" 3");
    fflush(stdout);
    data = &document2[10];
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 2 byte injection at beginning of area */
    test_ret += testDocumentRangeByte2(ctxt, &document2[0], strlen(document2),
                           data);
    printf(" 4");
    fflush(stdout);
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    /* test 2 byte injection at end of area */
    test_ret += testDocumentRangeByte2(ctxt, &document2[0], strlen(document2),
                           data + 2);
    printf(" done\n");

    xmlFreeParserCtxt(ctxt);
    return(test_ret);
}

static int testCharRangeByte1(xmlParserCtxtPtr ctxt) {
    int i = 0;
    int len, c;
    char *data = (char *) ctxt->input->cur;

    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    for (i = 0;i <= 0xFF;i++) {
        data[0] = (char) i;
	ctxt->charset = XML_CHAR_ENCODING_UTF8;
        ctxt->nbErrors = 0;

	lastError = 0;
        c = xmlCurrentChar(ctxt, &len);
	if ((i == 0) || (i >= 0x80)) {
	    /* we must see an error there */
	    if (lastError != XML_ERR_INVALID_CHAR) {
	        fprintf(stderr,
		    "Failed to detect invalid char for Byte 0x%02X\n", i);
		return(1);
	    }
	} else if (i == 0xD) {
	    if ((c != 0xA) || (len != 1)) {
		fprintf(stderr, "Failed to convert char for Byte 0x%02X\n", i);
		return(1);
	    }
	} else if ((c != i) || (len != 1)) {
	    fprintf(stderr, "Failed to parse char for Byte 0x%02X\n", i);
	    return(1);
	}
    }
    return(0);
}

static int testCharRangeByte2(xmlParserCtxtPtr ctxt) {
    int i, j;
    int len, c;
    char *data = (char *) ctxt->input->cur;

    data[2] = 0;
    data[3] = 0;
    for (i = 0x80;i <= 0xFF;i++) {
	for (j = 0;j <= 0xFF;j++) {
	    data[0] = (char) i;
	    data[1] = (char) j;
	    ctxt->charset = XML_CHAR_ENCODING_UTF8;
            ctxt->nbErrors = 0;

	    lastError = 0;
	    c = xmlCurrentChar(ctxt, &len);

	    /* if first bit of first char is set, then second bit must too */
	    if ((i & 0x80) && ((i & 0x40) == 0)) {
		if (lastError != XML_ERR_INVALID_CHAR) {
		    fprintf(stderr,
		    "Failed to detect invalid char for Bytes 0x%02X 0x%02X\n",
		            i, j);
		    return(1);
		}
	    }

	    /*
	     * if first bit of first char is set, then second char first
	     * bits must be 10
	     */
	    else if ((i & 0x80) && ((j & 0xC0) != 0x80)) {
		if (lastError != XML_ERR_INVALID_CHAR) {
		    fprintf(stderr,
		"Failed to detect invalid char for Bytes 0x%02X 0x%02X: %d\n",
		            i, j, c);
		    return(1);
		}
	    }

	    /*
	     * if using a 2 byte encoding then the value must be greater
	     * than 0x80, i.e. one of bits 5 to 1 of i must be set
	     */
	    else if ((i & 0x80) && ((i & 0x1E) == 0)) {
		if (lastError != XML_ERR_INVALID_CHAR) {
		    fprintf(stderr,
		"Failed to detect invalid char for Bytes 0x%02X 0x%02X: %d\n",
		            i, j, c);
		    return(1);
		}
	    }

	    /*
	     * if third bit of first char is set, then the sequence would need
	     * at least 3 bytes, but we give only 2 !
	     */
	    else if ((i & 0xE0) == 0xE0) {
		if (lastError != XML_ERR_INVALID_CHAR) {
		    fprintf(stderr,
		"Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x00\n",
		            i, j);
		    return(1);
		}
	    }

            /*
	     * We should see no error in remaining cases
	     */
	    else if ((lastError != 0) || (len != 2)) {
		fprintf(stderr,
		    "Failed to parse char for Bytes 0x%02X 0x%02X\n", i, j);
		return(1);
	    }

            /*
	     * Finally check the value is right
	     */
	    else if (c != (j & 0x3F) + ((i & 0x1F) << 6)) {
		fprintf(stderr,
	"Failed to parse char for Bytes 0x%02X 0x%02X: expect %d got %d\n",
	                i, j, ((j & 0x3F) + ((i & 0x1F) << 6)), c);
		return(1);
	    }
        }
    }
    return(0);
}

static int testCharRangeByte3(xmlParserCtxtPtr ctxt) {
    int i, j, k, K;
    int len, c;
    unsigned char lows[6] = {0, 0x80, 0x81, 0xC1, 0xFF, 0xBF};
    char *data = (char *) ctxt->input->cur;
    int value;

    data[3] = 0;
    for (i = 0xE0;i <= 0xFF;i++) {
    for (j = 0;j <= 0xFF;j++) {
    for (k = 0;k < 6;k++) {
	data[0] = (char) i;
	data[1] = (char) j;
	K = lows[k];
	data[2] = (char) K;
	value = (K & 0x3F) + ((j & 0x3F) << 6) + ((i & 0xF) << 12);
	ctxt->charset = XML_CHAR_ENCODING_UTF8;
        ctxt->nbErrors = 0;

	lastError = 0;
	c = xmlCurrentChar(ctxt, &len);

	/*
	 * if fourth bit of first char is set, then the sequence would need
	 * at least 4 bytes, but we give only 3 !
	 */
	if ((i & 0xF0) == 0xF0) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	"Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X 0x%02X\n",
			i, j, K, data[3]);
		return(1);
	    }
	}

        /*
	 * The second and the third bytes must start with 10
	 */
	else if (((j & 0xC0) != 0x80) || ((K & 0xC0) != 0x80)) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	"Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X\n",
			i, j, K);
		return(1);
	    }
	}

	/*
	 * if using a 3 byte encoding then the value must be greater
	 * than 0x800, i.e. one of bits 4 to 0 of i must be set or
	 * the 6th byte of data[1] must be set
	 */
	else if (((i & 0xF) == 0) && ((j & 0x20) == 0)) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	    "Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X\n",
			i, j, K);
		return(1);
	    }
	}

        /*
	 * There are values in that range that are not allowed in XML-1.0
	 */
	else if (((value > 0xD7FF) && (value <0xE000)) ||
	         ((value > 0xFFFD) && (value <0x10000))) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	"Failed to detect invalid char 0x%04X for Bytes 0x%02X 0x%02X 0x%02X\n",
			value, i, j, K);
		return(1);
	    }
	}

	/*
	 * We should see no error in remaining cases
	 */
	else if ((lastError != 0) || (len != 3)) {
	    fprintf(stderr,
		"Failed to parse char for Bytes 0x%02X 0x%02X 0x%02X\n",
		    i, j, K);
	    return(1);
	}

	/*
	 * Finally check the value is right
	 */
	else if (c != value) {
	    fprintf(stderr,
    "Failed to parse char for Bytes 0x%02X 0x%02X 0x%02X: expect %d got %d\n",
		i, j, data[2], value, c);
	    return(1);
	}
    }
    }
    }
    return(0);
}

static int testCharRangeByte4(xmlParserCtxtPtr ctxt) {
    int i, j, k, K, l, L;
    int len, c;
    unsigned char lows[6] = {0, 0x80, 0x81, 0xC1, 0xFF, 0xBF};
    char *data = (char *) ctxt->input->cur;
    int value;

    data[4] = 0;
    for (i = 0xF0;i <= 0xFF;i++) {
    for (j = 0;j <= 0xFF;j++) {
    for (k = 0;k < 6;k++) {
    for (l = 0;l < 6;l++) {
	data[0] = (char) i;
	data[1] = (char) j;
	K = lows[k];
	data[2] = (char) K;
	L = lows[l];
	data[3] = (char) L;
	value = (L & 0x3F) + ((K & 0x3F) << 6) + ((j & 0x3F) << 12) +
	        ((i & 0x7) << 18);
	ctxt->charset = XML_CHAR_ENCODING_UTF8;
        ctxt->nbErrors = 0;

	lastError = 0;
	c = xmlCurrentChar(ctxt, &len);

	/*
	 * if fifth bit of first char is set, then the sequence would need
	 * at least 5 bytes, but we give only 4 !
	 */
	if ((i & 0xF8) == 0xF8) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
  "Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X 0x%02X\n",
			i, j, K, data[3]);
		return(1);
	    }
	}

        /*
	 * The second, third and fourth bytes must start with 10
	 */
	else if (((j & 0xC0) != 0x80) || ((K & 0xC0) != 0x80) ||
	         ((L & 0xC0) != 0x80)) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	"Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X 0x%02X\n",
			i, j, K, L);
		return(1);
	    }
	}

	/*
	 * if using a 3 byte encoding then the value must be greater
	 * than 0x10000, i.e. one of bits 3 to 0 of i must be set or
	 * the 6 or 5th byte of j must be set
	 */
	else if (((i & 0x7) == 0) && ((j & 0x30) == 0)) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
	"Failed to detect invalid char for Bytes 0x%02X 0x%02X 0x%02X 0x%02X\n",
			i, j, K, L);
		return(1);
	    }
	}

        /*
	 * There are values in that range that are not allowed in XML-1.0
	 */
	else if (((value > 0xD7FF) && (value <0xE000)) ||
	         ((value > 0xFFFD) && (value <0x10000)) ||
		 (value > 0x10FFFF)) {
	    if (lastError != XML_ERR_INVALID_CHAR) {
		fprintf(stderr,
"Failed to detect invalid char 0x%04X for Bytes 0x%02X 0x%02X 0x%02X 0x%02X\n",
			value, i, j, K, L);
		return(1);
	    }
	}

	/*
	 * We should see no error in remaining cases
	 */
	else if ((lastError != 0) || (len != 4)) {
	    fprintf(stderr,
		"Failed to parse char for Bytes 0x%02X 0x%02X 0x%02X\n",
		    i, j, K);
	    return(1);
	}

	/*
	 * Finally check the value is right
	 */
	else if (c != value) {
	    fprintf(stderr,
    "Failed to parse char for Bytes 0x%02X 0x%02X 0x%02X: expect %d got %d\n",
		i, j, data[2], value, c);
	    return(1);
	}
    }
    }
    }
    }
    return(0);
}

/**
 * testCharRanges:
 *
 * Test the correct UTF8 character parsing in isolation i.e.
 * not when parsing a full document, this is less expensive and we can
 * cover the full range of UTF-8 chars accepted by XML-1.0
 */

static int testCharRanges(void) {
    char data[5];
    xmlParserCtxtPtr ctxt;
    xmlParserInputBufferPtr buf;
    xmlParserInputPtr input;
    int test_ret = 0;

    memset(data, 0, 5);

    /*
     * Set up a parsing context using the above data buffer as
     * the current input source.
     */
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
	return(1);
    }
    buf = xmlParserInputBufferCreateMem(data, sizeof(data),
                                        XML_CHAR_ENCODING_NONE);
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate input buffer\n");
	test_ret = 1;
	goto error;
    }
    input = xmlNewInputStream(ctxt);
    if (input == NULL) {
        xmlFreeParserInputBuffer(buf);
	test_ret = 1;
	goto error;
    }
    input->filename = NULL;
    input->buf = buf;
    input->cur =
    input->base = xmlBufContent(input->buf->buffer);
    input->end = input->base + 4;
    inputPush(ctxt, input);

    printf("testing char range: 1");
    fflush(stdout);
    test_ret += testCharRangeByte1(ctxt);
    printf(" 2");
    fflush(stdout);
    test_ret += testCharRangeByte2(ctxt);
    printf(" 3");
    fflush(stdout);
    test_ret += testCharRangeByte3(ctxt);
    printf(" 4");
    fflush(stdout);
    test_ret += testCharRangeByte4(ctxt);
    printf(" done\n");
    fflush(stdout);

error:
    xmlFreeParserCtxt(ctxt);
    return(test_ret);
}

static int
testUserEncoding(void) {
    /*
     * Create a document encoded as UTF-16LE with an ISO-8859-1 encoding
     * declaration, then parse it with xmlReadMemory and the encoding
     * argument set to UTF-16LE.
     */
    xmlDocPtr doc = NULL;
    const char *start = "<?xml version='1.0' encoding='ISO-8859-1'?><d>";
    const char *end = "</d>";
    char *buf = NULL;
    xmlChar *text;
    int startSize = strlen(start);
    int textSize = 100000; /* Make sure to exceed internal buffer sizes. */
    int endSize = strlen(end);
    int totalSize = startSize + textSize + endSize;
    int k = 0;
    int i;
    int ret = 1;

    buf = xmlMalloc(2 * totalSize);
    for (i = 0; start[i] != 0; i++) {
        buf[k++] = start[i];
        buf[k++] = 0;
    }
    for (i = 0; i < textSize; i++) {
        buf[k++] = 'x';
        buf[k++] = 0;
    }
    for (i = 0; end[i] != 0; i++) {
        buf[k++] = end[i];
        buf[k++] = 0;
    }

    doc = xmlReadMemory(buf, 2 * totalSize, NULL, "UTF-16LE", 0);
    if (doc == NULL) {
        fprintf(stderr, "failed to parse document\n");
        goto error;
    }

    text = doc->children->children->content;
    for (i = 0; i < textSize; i++) {
        if (text[i] != 'x') {
            fprintf(stderr, "text node has wrong content at offset %d\n", k);
            goto error;
        }
    }

    ret = 0;

error:
    xmlFreeDoc(doc);
    xmlFree(buf);

    return ret;
}

static int
testUTF8Chunks(void) {
#if defined(LIBXML_PUSH_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
    xmlParserCtxtPtr ctxt;
    xmlChar *out;
    int outSize;
    char *buf;
    int i;
    int ret = 0;

    ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);

    xmlParseChunk(ctxt, "<d>", 3, 0);
    xmlParseChunk(ctxt, "\xF0", 1, 0);
    xmlParseChunk(ctxt, "\x9F", 1, 0);
    xmlParseChunk(ctxt, "\x98", 1, 0);
    xmlParseChunk(ctxt, "\x8A", 1, 0);
    xmlParseChunk(ctxt, "</d>", 4, 1);

    xmlDocDumpMemory(ctxt->myDoc, &out, &outSize);
    if (strcmp((char *) out,
               "<?xml version=\"1.0\"?>\n<d>&#x1F60A;</d>\n") != 0) {
        fprintf(stderr, "failed UTF-8 chunk test 1\n");
        ret += 1;
    }

    xmlFree(out);
    xmlFreeDoc(ctxt->myDoc);
    xmlFreeParserCtxt(ctxt);

    ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);

    xmlParseChunk(ctxt, "<d>", 3, 0);

    /*
     * Create a chunk longer than XML_PARSER_BIG_BUFFER_SIZE (300) ending
     * with an incomplete UTF-8 sequence.
     */
    buf = xmlMalloc(1000 * 2 + 1);
    for (i = 0; i < 2000; i += 2)
        memcpy(buf + i, "\xCE\xB1", 2);
    buf[i] = '\xCE';
    xmlParseChunk(ctxt, buf, 2001, 0);
    xmlFree(buf);

    xmlParseChunk(ctxt, "\xB1</d>", 4, 0);
    xmlParseChunk(ctxt, NULL, 0, 0);

    xmlDocDumpMemory(ctxt->myDoc, &out, &outSize);
    if (strncmp((char *) out, "<?xml version=\"1.0\"?>\n<d>", 25) != 0) {
        fprintf(stderr, "failed UTF-8 chunk test 2-1\n");
        ret += 1;
        goto error;
    }
    for (i = 25; i < 25 + 1001 * 7; i += 7) {
        if (memcmp(out + i, "&#x3B1;", 7) != 0) {
            fprintf(stderr, "failed UTF-8 chunk test 2-2 %d\n", i);
            ret += 1;
            goto error;
        }
    }
    if (strcmp((char *) out + i, "</d>\n") != 0) {
        fprintf(stderr, "failed UTF-8 chunk test 2-3\n");
        ret += 1;
        goto error;
    }

error:
    xmlFree(out);
    xmlFreeDoc(ctxt->myDoc);
    xmlFreeParserCtxt(ctxt);

    return(ret);
#else
    return(0);
#endif
}

int main(void) {

    int ret = 0;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*
     * Catch errors separately
     */

    xmlSetStructuredErrorFunc(NULL, errorHandler);

    /*
     * Run the tests
     */
    ret += testCharRanges();
    ret += testDocumentRanges();
    ret += testUserEncoding();
    ret += testUTF8Chunks();

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    return(ret ? 1 : 0);
}
