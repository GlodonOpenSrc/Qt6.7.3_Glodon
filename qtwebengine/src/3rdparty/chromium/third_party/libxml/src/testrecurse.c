/*
 * testrecurse.c: C program to run libxml2 regression tests checking entities
 *            recursions
 *
 * To compile on Unixes:
 * cc -o testrecurse `xml2-config --cflags` testrecurse.c `xml2-config --libs` -lpthread
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxml.h"
#include <stdio.h>

#include <string.h>
#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/uri.h>

/*
 * O_BINARY is just for Windows compatibility - if it isn't defined
 * on this system, avoid any compilation error
 */
#ifdef	O_BINARY
#define RD_FLAGS	O_RDONLY | O_BINARY
#else
#define	RD_FLAGS	O_RDONLY
#endif

#define OPT_SAX         (1<<0)
#define OPT_NO_SUBST    (1<<1)

typedef int (*functest) (const char *filename, const char *result,
                         const char *error, int options);

typedef struct testDesc testDesc;
typedef testDesc *testDescPtr;
struct testDesc {
    const char *desc; /* description of the test */
    functest    func; /* function implementing the test */
    const char *in;   /* glob to path for input files */
    const char *out;  /* output directory */
    const char *suffix;/* suffix for output files */
    const char *err;  /* suffix for error output files */
    int     options;  /* parser options for the test */
};

static int checkTestFile(const char *filename);


#if defined(_WIN32)

#include <windows.h>

typedef struct
{
      size_t gl_pathc;    /* Count of paths matched so far  */
      char **gl_pathv;    /* List of matched pathnames.  */
      size_t gl_offs;     /* Slots to reserve in 'gl_pathv'.  */
} glob_t;

#define GLOB_DOOFFS 0
static int glob(const char *pattern, ATTRIBUTE_UNUSED int flags,
                ATTRIBUTE_UNUSED int errfunc(const char *epath, int eerrno),
                glob_t *pglob) {
    glob_t *ret;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    unsigned int nb_paths = 0;
    char directory[500];
    int len;

    if ((pattern == NULL) || (pglob == NULL)) return(-1);

    strncpy(directory, pattern, 499);
    for (len = strlen(directory);len >= 0;len--) {
        if (directory[len] == '/') {
	    len++;
	    directory[len] = 0;
	    break;
	}
    }
    if (len <= 0)
        len = 0;


    ret = pglob;
    memset(ret, 0, sizeof(glob_t));

    hFind = FindFirstFileA(pattern, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
        return(0);
    nb_paths = 20;
    ret->gl_pathv = (char **) malloc(nb_paths * sizeof(char *));
    if (ret->gl_pathv == NULL) {
	FindClose(hFind);
        return(-1);
    }
    strncpy(directory + len, FindFileData.cFileName, 499 - len);
    ret->gl_pathv[ret->gl_pathc] = strdup(directory);
    if (ret->gl_pathv[ret->gl_pathc] == NULL)
        goto done;
    ret->gl_pathc++;
    while(FindNextFileA(hFind, &FindFileData)) {
        if (FindFileData.cFileName[0] == '.')
	    continue;
        if (ret->gl_pathc + 2 > nb_paths) {
            char **tmp = realloc(ret->gl_pathv, nb_paths * 2 * sizeof(char *));
            if (tmp == NULL)
                break;
            ret->gl_pathv = tmp;
            nb_paths *= 2;
	}
	strncpy(directory + len, FindFileData.cFileName, 499 - len);
	ret->gl_pathv[ret->gl_pathc] = strdup(directory);
        if (ret->gl_pathv[ret->gl_pathc] == NULL)
            break;
        ret->gl_pathc++;
    }
    ret->gl_pathv[ret->gl_pathc] = NULL;

done:
    FindClose(hFind);
    return(0);
}



static void globfree(glob_t *pglob) {
    unsigned int i;
    if (pglob == NULL)
        return;

    for (i = 0;i < pglob->gl_pathc;i++) {
         if (pglob->gl_pathv[i] != NULL)
             free(pglob->gl_pathv[i]);
    }
}

#else
#include <glob.h>
#endif

/************************************************************************
 *									*
 *		Huge document generator					*
 *									*
 ************************************************************************/

#include <libxml/xmlIO.h>

typedef struct {
    const char *URL;
    const char *start;
    const char *segment;
    const char *finish;
} xmlHugeDocParts;

static const xmlHugeDocParts hugeDocTable[] = {
    {
        "test/recurse/huge.xml",

        "<!DOCTYPE foo ["
        "<!ELEMENT foo (bar*)> "
        "<!ELEMENT bar (#PCDATA)> "
        "<!ATTLIST bar attr CDATA #IMPLIED> "
        "<!ENTITY a SYSTEM 'ga.ent'> "
        "<!ENTITY b SYSTEM 'gb.ent'> "
        "<!ENTITY c SYSTEM 'gc.ent'> "
        "<!ENTITY f 'some internal data'> "
        "<!ENTITY e '&f;&f;'> "
        "<!ENTITY d '&e;&e;'> "
        "]> "
        "<foo>",

        "  <bar attr='&e; &f; &d;'>&a; &b; &c; &e; &f; &d;</bar>\n"
        "  <bar>_123456789_123456789_123456789_123456789</bar>\n"
        "  <bar>_123456789_123456789_123456789_123456789</bar>\n"
        "  <bar>_123456789_123456789_123456789_123456789</bar>\n"
        "  <bar>_123456789_123456789_123456789_123456789</bar>\n",

        "</foo>"
    },
    {
        "test/recurse/huge_dtd.dtd",

        "<!ELEMENT foo (#PCDATA)>\n"
        "<!ENTITY ent 'success'>\n"
        "<!ENTITY % a SYSTEM 'pa.ent'>\n"
        "<!ENTITY % b SYSTEM 'pb.ent'>\n"
        "<!ENTITY % c SYSTEM 'pc.ent'>\n"
        "<!ENTITY % d '<!-- comment -->'>\n"
        "<!ENTITY % e '%d;%d;'>\n"
        "<!ENTITY % f '%e;%e;'>\n",

        "<!ENTITY ent '%a; %b; %c; %d; %e; %f;'>\n"
        "%a; %b; %c; %d; %e; %f;\n"
        "<!-- _123456789_123456789_123456789_123456789 -->\n"
        "<!-- _123456789_123456789_123456789_123456789 -->\n"
        "<!-- _123456789_123456789_123456789_123456789 -->\n",

        ""
    },
    { NULL, NULL, NULL, NULL }
};

static const xmlHugeDocParts *hugeDocParts;
static int curseg = 0;
static const char *current;
static int rlen;

/**
 * hugeMatch:
 * @URI: an URI to test
 *
 * Check for a huge query
 *
 * Returns 1 if yes and 0 if another Input module should be used
 */
static int
hugeMatch(const char * URI) {
    int i;

    if (URI == NULL)
        return(0);

    for (i = 0; hugeDocTable[i].URL; i++) {
        if (strcmp(URI, hugeDocTable[i].URL) == 0)
            return(1);
    }

    return(0);
}

/**
 * hugeOpen:
 * @URI: an URI to test
 *
 * Return a pointer to the huge query handler, in this example simply
 * the current pointer...
 *
 * Returns an Input context or NULL in case or error
 */
static void *
hugeOpen(const char * URI) {
    int i;

    if (URI == NULL)
        return(NULL);

    for (i = 0; hugeDocTable[i].URL; i++) {
        if (strcmp(URI, hugeDocTable[i].URL) == 0) {
            hugeDocParts = hugeDocTable + i;
            curseg = 0;
            current = hugeDocParts->start;
            rlen = strlen(current);
            return((void *) current);
        }
    }

    return(NULL);
}

/**
 * hugeClose:
 * @context: the read context
 *
 * Close the huge query handler
 *
 * Returns 0 or -1 in case of error
 */
static int
hugeClose(void * context) {
    if (context == NULL) return(-1);
    return(0);
}

#define MAX_NODES 1000

/**
 * hugeRead:
 * @context: the read context
 * @buffer: where to store data
 * @len: number of bytes to read
 *
 * Implement an huge query read.
 *
 * Returns the number of bytes read or -1 in case of error
 */
static int
hugeRead(void *context, char *buffer, int len)
{
    if ((context == NULL) || (buffer == NULL) || (len < 0))
        return (-1);

    if (len >= rlen) {
        if (curseg >= MAX_NODES + 1) {
            rlen = 0;
            return(0);
        }
        len = rlen;
        rlen = 0;
	memcpy(buffer, current, len);
        curseg ++;
        if (curseg == MAX_NODES) {
            current = hugeDocParts->finish;
	} else {
            current = hugeDocParts->segment;
	}
        rlen = strlen(current);
    } else {
	memcpy(buffer, current, len);
	rlen -= len;
        current += len;
    }
    return (len);
}

/************************************************************************
 *									*
 *		Libxml2 specific routines				*
 *									*
 ************************************************************************/

static int nb_tests = 0;
static int nb_errors = 0;
static int nb_leaks = 0;
static int extraMemoryFromResolver = 0;

static int
fatalError(void) {
    fprintf(stderr, "Exitting tests on fatal error\n");
    exit(1);
}

/*
 * We need to trap calls to the resolver to not account memory for the catalog
 * which is shared to the current running test. We also don't want to have
 * network downloads modifying tests.
 */
static xmlParserInputPtr
testExternalEntityLoader(const char *URL, const char *ID,
			 xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr ret;

    if (checkTestFile(URL)) {
	ret = xmlNoNetExternalEntityLoader(URL, ID, ctxt);
    } else {
	int memused = xmlMemUsed();
	ret = xmlNoNetExternalEntityLoader(URL, ID, ctxt);
	extraMemoryFromResolver += xmlMemUsed() - memused;
    }

    return(ret);
}

/*
 * Trapping the error messages at the generic level to grab the equivalent of
 * stderr messages on CLI tools.
 */
static char testErrors[32769];
static int testErrorsSize = 0;

static void
channel(void *ctx  ATTRIBUTE_UNUSED, const char *msg, ...) {
    va_list args;
    int res;

    if (testErrorsSize >= 32768)
        return;
    va_start(args, msg);
    res = vsnprintf(&testErrors[testErrorsSize],
                    32768 - testErrorsSize,
		    msg, args);
    va_end(args);
    if (testErrorsSize + res >= 32768) {
        /* buffer is full */
	testErrorsSize = 32768;
	testErrors[testErrorsSize] = 0;
    } else {
        testErrorsSize += res;
    }
    testErrors[testErrorsSize] = 0;
}

/**
 * xmlParserPrintFileContext:
 * @input:  an xmlParserInputPtr input
 *
 * Displays current context within the input content for error tracking
 */

static void
xmlParserPrintFileContextInternal(xmlParserInputPtr input ,
		xmlGenericErrorFunc chanl, void *data ) {
    const xmlChar *cur, *base;
    unsigned int n, col;	/* GCC warns if signed, because compared with sizeof() */
    xmlChar  content[81]; /* space for 80 chars + line terminator */
    xmlChar *ctnt;

    if (input == NULL) return;
    cur = input->cur;
    base = input->base;
    /* skip backwards over any end-of-lines */
    while ((cur > base) && ((*(cur) == '\n') || (*(cur) == '\r'))) {
	cur--;
    }
    n = 0;
    /* search backwards for beginning-of-line (to max buff size) */
    while ((n++ < (sizeof(content)-1)) && (cur > base) &&
   (*(cur) != '\n') && (*(cur) != '\r'))
        cur--;
    if ((*(cur) == '\n') || (*(cur) == '\r')) cur++;
    /* calculate the error position in terms of the current position */
    col = input->cur - cur;
    /* search forward for end-of-line (to max buff size) */
    n = 0;
    ctnt = content;
    /* copy selected text to our buffer */
    while ((*cur != 0) && (*(cur) != '\n') &&
   (*(cur) != '\r') && (n < sizeof(content)-1)) {
		*ctnt++ = *cur++;
	n++;
    }
    *ctnt = 0;
    /* print out the selected text */
    chanl(data ,"%s\n", content);
    /* create blank line with problem pointer */
    n = 0;
    ctnt = content;
    /* (leave buffer space for pointer + line terminator) */
    while ((n<col) && (n++ < sizeof(content)-2) && (*ctnt != 0)) {
	if (*(ctnt) != '\t')
	    *(ctnt) = ' ';
	ctnt++;
    }
    *ctnt++ = '^';
    *ctnt = 0;
    chanl(data ,"%s\n", content);
}

static void
testStructuredErrorHandler(void *ctx  ATTRIBUTE_UNUSED, const xmlError *err) {
    char *file = NULL;
    int line = 0;
    int code = -1;
    int domain;
    void *data = NULL;
    const char *str;
    const xmlChar *name = NULL;
    xmlNodePtr node;
    xmlErrorLevel level;
    xmlParserInputPtr input = NULL;
    xmlParserInputPtr cur = NULL;
    xmlParserCtxtPtr ctxt = NULL;

    if (err == NULL)
        return;

    file = err->file;
    line = err->line;
    code = err->code;
    domain = err->domain;
    level = err->level;
    node = err->node;
    if ((domain == XML_FROM_PARSER) || (domain == XML_FROM_HTML) ||
        (domain == XML_FROM_DTD) || (domain == XML_FROM_NAMESPACE) ||
	(domain == XML_FROM_IO) || (domain == XML_FROM_VALID)) {
	ctxt = err->ctxt;
    }
    str = err->message;

    if (code == XML_ERR_OK)
        return;

    if ((node != NULL) && (node->type == XML_ELEMENT_NODE))
        name = node->name;

    /*
     * Maintain the compatibility with the legacy error handling
     */
    if (ctxt != NULL) {
        input = ctxt->input;
        if ((input != NULL) && (input->filename == NULL) &&
            (ctxt->inputNr > 1)) {
            cur = input;
            input = ctxt->inputTab[ctxt->inputNr - 2];
        }
        if (input != NULL) {
            if (input->filename)
                channel(data, "%s:%d: ", input->filename, input->line);
            else if ((line != 0) && (domain == XML_FROM_PARSER))
                channel(data, "Entity: line %d: ", input->line);
        }
    } else {
        if (file != NULL)
            channel(data, "%s:%d: ", file, line);
        else if ((line != 0) && (domain == XML_FROM_PARSER))
            channel(data, "Entity: line %d: ", line);
    }
    if (name != NULL) {
        channel(data, "element %s: ", name);
    }
    if (code == XML_ERR_OK)
        return;
    switch (domain) {
        case XML_FROM_PARSER:
            channel(data, "parser ");
            break;
        case XML_FROM_NAMESPACE:
            channel(data, "namespace ");
            break;
        case XML_FROM_DTD:
        case XML_FROM_VALID:
            channel(data, "validity ");
            break;
        case XML_FROM_HTML:
            channel(data, "HTML parser ");
            break;
        case XML_FROM_MEMORY:
            channel(data, "memory ");
            break;
        case XML_FROM_OUTPUT:
            channel(data, "output ");
            break;
        case XML_FROM_IO:
            channel(data, "I/O ");
            break;
        case XML_FROM_XINCLUDE:
            channel(data, "XInclude ");
            break;
        case XML_FROM_XPATH:
            channel(data, "XPath ");
            break;
        case XML_FROM_XPOINTER:
            channel(data, "parser ");
            break;
        case XML_FROM_REGEXP:
            channel(data, "regexp ");
            break;
        case XML_FROM_MODULE:
            channel(data, "module ");
            break;
        case XML_FROM_SCHEMASV:
            channel(data, "Schemas validity ");
            break;
        case XML_FROM_SCHEMASP:
            channel(data, "Schemas parser ");
            break;
        case XML_FROM_RELAXNGP:
            channel(data, "Relax-NG parser ");
            break;
        case XML_FROM_RELAXNGV:
            channel(data, "Relax-NG validity ");
            break;
        case XML_FROM_CATALOG:
            channel(data, "Catalog ");
            break;
        case XML_FROM_C14N:
            channel(data, "C14N ");
            break;
        case XML_FROM_XSLT:
            channel(data, "XSLT ");
            break;
        default:
            break;
    }
    if (code == XML_ERR_OK)
        return;
    switch (level) {
        case XML_ERR_NONE:
            channel(data, ": ");
            break;
        case XML_ERR_WARNING:
            channel(data, "warning : ");
            break;
        case XML_ERR_ERROR:
            channel(data, "error : ");
            break;
        case XML_ERR_FATAL:
            channel(data, "error : ");
            break;
    }
    if (code == XML_ERR_OK)
        return;
    if (str != NULL) {
        int len;
	len = xmlStrlen((const xmlChar *)str);
	if ((len > 0) && (str[len - 1] != '\n'))
	    channel(data, "%s\n", str);
	else
	    channel(data, "%s", str);
    } else {
        channel(data, "%s\n", "out of memory error");
    }
    if (code == XML_ERR_OK)
        return;

    if (ctxt != NULL) {
        xmlParserPrintFileContextInternal(input, channel, data);
        if (cur != NULL) {
            if (cur->filename)
                channel(data, "%s:%d: \n", cur->filename, cur->line);
            else if ((line != 0) && (domain == XML_FROM_PARSER))
                channel(data, "Entity: line %d: \n", cur->line);
            xmlParserPrintFileContextInternal(cur, channel, data);
        }
    }
    if ((domain == XML_FROM_XPATH) && (err->str1 != NULL) &&
        (err->int1 < 100) &&
	(err->int1 < xmlStrlen((const xmlChar *)err->str1))) {
	xmlChar buf[150];
	int i;

	channel(data, "%s\n", err->str1);
	for (i=0;i < err->int1;i++)
	     buf[i] = ' ';
	buf[i++] = '^';
	buf[i] = 0;
	channel(data, "%s\n", buf);
    }
}

static void
initializeLibxml2(void) {
    xmlGetWarningsDefaultValue = 0;
    xmlPedanticParserDefault(0);

    xmlMemSetup(xmlMemFree, xmlMemMalloc, xmlMemRealloc, xmlMemoryStrdup);
    xmlInitParser();
    xmlSetExternalEntityLoader(testExternalEntityLoader);
    xmlSetStructuredErrorFunc(NULL, testStructuredErrorHandler);
    /*
     * register the new I/O handlers
     */
    if (xmlRegisterInputCallbacks(hugeMatch, hugeOpen,
                                  hugeRead, hugeClose) < 0) {
        fprintf(stderr, "failed to register Huge handler\n");
	exit(1);
    }
}

static void
initSAX(xmlParserCtxtPtr ctxt) {
    ctxt->sax->startElementNs = NULL;
    ctxt->sax->endElementNs = NULL;
    ctxt->sax->characters = NULL;
    ctxt->sax->cdataBlock = NULL;
    ctxt->sax->ignorableWhitespace = NULL;
    ctxt->sax->processingInstruction = NULL;
    ctxt->sax->comment = NULL;
}

/************************************************************************
 *									*
 *		File name and path utilities				*
 *									*
 ************************************************************************/

static const char *baseFilename(const char *filename) {
    const char *cur;
    if (filename == NULL)
        return(NULL);
    cur = &filename[strlen(filename)];
    while ((cur > filename) && (*cur != '/'))
        cur--;
    if (*cur == '/')
        return(cur + 1);
    return(cur);
}

static char *resultFilename(const char *filename, const char *out,
                            const char *suffix) {
    const char *base;
    char res[500];
    char suffixbuff[500];

/*************
    if ((filename[0] == 't') && (filename[1] == 'e') &&
        (filename[2] == 's') && (filename[3] == 't') &&
	(filename[4] == '/'))
	filename = &filename[5];
 *************/

    base = baseFilename(filename);
    if (suffix == NULL)
        suffix = ".tmp";
    if (out == NULL)
        out = "";

    strncpy(suffixbuff,suffix,499);
#ifdef VMS
    if(strstr(base,".") && suffixbuff[0]=='.')
      suffixbuff[0]='_';
#endif

    if (snprintf(res, 499, "%s%s%s", out, base, suffixbuff) >= 499)
        res[499] = 0;
    return(strdup(res));
}

static int checkTestFile(const char *filename) {
    struct stat buf;

    if (stat(filename, &buf) == -1)
        return(0);

#if defined(_WIN32)
    if (!(buf.st_mode & _S_IFREG))
        return(0);
#else
    if (!S_ISREG(buf.st_mode))
        return(0);
#endif

    return(1);
}



/************************************************************************
 *									*
 *		Test to detect or not recursive entities		*
 *									*
 ************************************************************************/
/**
 * recursiveDetectTest:
 * @filename: the file to parse
 * @result: the file with expected result
 * @err: the file with error messages: unused
 *
 * Parse a file loading DTD and replacing entities check it fails for
 * lol cases
 *
 * Returns 0 in case of success, an error code otherwise
 */
static int
recursiveDetectTest(const char *filename,
             const char *result ATTRIBUTE_UNUSED,
             const char *err ATTRIBUTE_UNUSED,
	     int options) {
    xmlDocPtr doc;
    xmlParserCtxtPtr ctxt;
    int res = 0;
    /*
     * XML_PARSE_DTDVALID is the only way to load external entities
     * without XML_PARSE_NOENT. The validation result doesn't matter
     * anyway.
     */
    int parserOptions = XML_PARSE_DTDVALID;

    nb_tests++;

    ctxt = xmlNewParserCtxt();
    if (options & OPT_SAX)
        initSAX(ctxt);
    if ((options & OPT_NO_SUBST) == 0)
        parserOptions |= XML_PARSE_NOENT;
    /*
     * base of the test, parse with the old API
     */
    doc = xmlCtxtReadFile(ctxt, filename, NULL, parserOptions);
    if ((doc != NULL) || (ctxt->lastError.code != XML_ERR_ENTITY_LOOP)) {
        fprintf(stderr, "Failed to detect recursion in %s\n", filename);
	xmlFreeParserCtxt(ctxt);
	xmlFreeDoc(doc);
        return(1);
    }
    xmlFreeParserCtxt(ctxt);

    return(res);
}

/**
 * notRecursiveDetectTest:
 * @filename: the file to parse
 * @result: the file with expected result
 * @err: the file with error messages: unused
 *
 * Parse a file loading DTD and replacing entities check it works for
 * good cases
 *
 * Returns 0 in case of success, an error code otherwise
 */
static int
notRecursiveDetectTest(const char *filename,
             const char *result ATTRIBUTE_UNUSED,
             const char *err ATTRIBUTE_UNUSED,
	     int options) {
    xmlDocPtr doc;
    xmlParserCtxtPtr ctxt;
    int res = 0;
    int parserOptions = XML_PARSE_DTDLOAD;

    nb_tests++;

    ctxt = xmlNewParserCtxt();
    if (options & OPT_SAX)
        initSAX(ctxt);
    if ((options & OPT_NO_SUBST) == 0)
        parserOptions |= XML_PARSE_NOENT;
    /*
     * base of the test, parse with the old API
     */
    doc = xmlCtxtReadFile(ctxt, filename, NULL, parserOptions);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse correct file %s\n", filename);
	xmlFreeParserCtxt(ctxt);
        return(1);
    }
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(ctxt);

    return(res);
}

/**
 * notRecursiveHugeTest:
 * @filename: the file to parse
 * @result: the file with expected result
 * @err: the file with error messages: unused
 *
 * Parse a memory generated file
 * good cases
 *
 * Returns 0 in case of success, an error code otherwise
 */
static int
notRecursiveHugeTest(const char *filename ATTRIBUTE_UNUSED,
             const char *result ATTRIBUTE_UNUSED,
             const char *err ATTRIBUTE_UNUSED,
	     int options) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;
    int res = 0;
    int parserOptions = XML_PARSE_DTDVALID;

    nb_tests++;

    ctxt = xmlNewParserCtxt();
    if (options & OPT_SAX)
        initSAX(ctxt);
    if ((options & OPT_NO_SUBST) == 0)
        parserOptions |= XML_PARSE_NOENT;
    doc = xmlCtxtReadFile(ctxt, "test/recurse/huge.xml", NULL, parserOptions);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse huge.xml\n");
	res = 1;
    } else {
        xmlEntityPtr ent;
        unsigned long fixed_cost = 20;
        unsigned long allowed_expansion = 1000000;
        unsigned long f_size = xmlStrlen(BAD_CAST "some internal data");
        unsigned long e_size;
        unsigned long d_size;
        unsigned long total_size;

        ent = xmlGetDocEntity(doc, BAD_CAST "e");
        e_size = f_size * 2 +
                 xmlStrlen(BAD_CAST "&f;") * 2 +
                 fixed_cost * 2;
        if (ent->expandedSize != e_size) {
            fprintf(stderr, "Wrong size for entity e: %lu (expected %lu)\n",
                    ent->expandedSize, e_size);
            res = 1;
        }

        ent = xmlGetDocEntity(doc, BAD_CAST "b");
        if (ent->expandedSize != e_size) {
            fprintf(stderr, "Wrong size for entity b: %lu (expected %lu)\n",
                    ent->expandedSize, e_size);
            res = 1;
        }

        ent = xmlGetDocEntity(doc, BAD_CAST "d");
        d_size = e_size * 2 +
                 xmlStrlen(BAD_CAST "&e;") * 2 +
                 fixed_cost * 2;
        if (ent->expandedSize != d_size) {
            fprintf(stderr, "Wrong size for entity d: %lu (expected %lu)\n",
                    ent->expandedSize, d_size);
            res = 1;
        }

        ent = xmlGetDocEntity(doc, BAD_CAST "c");
        if (ent->expandedSize != d_size) {
            fprintf(stderr, "Wrong size for entity c: %lu (expected %lu)\n",
                    ent->expandedSize, d_size);
            res = 1;
        }

        if (ctxt->sizeentcopy < allowed_expansion) {
            fprintf(stderr, "Total entity size too small: %lu\n",
                    ctxt->sizeentcopy);
            res = 1;
        }

        total_size = (f_size + e_size + d_size + 3 * fixed_cost) *
                     (MAX_NODES - 1) * 3;
        if (ctxt->sizeentcopy != total_size) {
            fprintf(stderr, "Wrong total entity size: %lu (expected %lu)\n",
                    ctxt->sizeentcopy, total_size);
            res = 1;
        }

        if (ctxt->sizeentities != 30) {
            fprintf(stderr, "Wrong parsed entity size: %lu (expected %lu)\n",
                    ctxt->sizeentities, 30lu);
            res = 1;
        }
    }

    xmlFreeDoc(doc);
    xmlFreeParserCtxt(ctxt);

    return(res);
}

/**
 * notRecursiveHugeTest:
 * @filename: the file to parse
 * @result: the file with expected result
 * @err: the file with error messages: unused
 *
 * Parse a memory generated file
 * good cases
 *
 * Returns 0 in case of success, an error code otherwise
 */
static int
hugeDtdTest(const char *filename ATTRIBUTE_UNUSED,
            const char *result ATTRIBUTE_UNUSED,
            const char *err ATTRIBUTE_UNUSED,
            int options) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;
    int res = 0;
    int parserOptions = XML_PARSE_DTDVALID;

    nb_tests++;

    ctxt = xmlNewParserCtxt();
    if (options & OPT_SAX)
        initSAX(ctxt);
    if ((options & OPT_NO_SUBST) == 0)
        parserOptions |= XML_PARSE_NOENT;
    doc = xmlCtxtReadFile(ctxt, "test/recurse/huge_dtd.xml", NULL,
                          parserOptions);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse huge_dtd.xml\n");
	res = 1;
    } else {
        unsigned long fixed_cost = 20;
        unsigned long allowed_expansion = 1000000;
        unsigned long a_size = xmlStrlen(BAD_CAST "<!-- comment -->");
        unsigned long b_size;
        unsigned long c_size;
        unsigned long e_size;
        unsigned long f_size;
        unsigned long total_size;

        if (ctxt->sizeentcopy < allowed_expansion) {
            fprintf(stderr, "Total entity size too small: %lu\n",
                    ctxt->sizeentcopy);
            res = 1;
        }

        b_size = (a_size + strlen("&a;") + fixed_cost) * 2;
        c_size = (b_size + strlen("&b;") + fixed_cost) * 2;
        /*
         * Internal parameter entites are substitued eagerly and
         * need different accounting.
         */
        e_size = a_size * 2;
        f_size = e_size * 2;
        total_size = /* internal */
                     e_size + f_size + fixed_cost * 4 +
                     (a_size + e_size + f_size + fixed_cost * 3) *
                     (MAX_NODES - 1) * 2 +
                     /* external */
                     (a_size + b_size + c_size + fixed_cost * 3) *
                     (MAX_NODES - 1) * 2 +
                     /* final reference in main doc */
                     strlen("success") + fixed_cost;
        if (ctxt->sizeentcopy != total_size) {
            fprintf(stderr, "Wrong total entity size: %lu (expected %lu)\n",
                    ctxt->sizeentcopy, total_size);
            res = 1;
        }

        total_size = strlen(hugeDocParts->start) +
                     strlen(hugeDocParts->segment) * (MAX_NODES - 1) +
                     strlen(hugeDocParts->finish) +
                     28;
        if (ctxt->sizeentities != total_size) {
            fprintf(stderr, "Wrong parsed entity size: %lu (expected %lu)\n",
                    ctxt->sizeentities, total_size);
            res = 1;
        }
    }

    xmlFreeDoc(doc);
    xmlFreeParserCtxt(ctxt);

    return(res);
}

/************************************************************************
 *									*
 *			Tests Descriptions				*
 *									*
 ************************************************************************/

static
testDesc testDescriptions[] = {
    { "Parsing recursive test cases" ,
      recursiveDetectTest, "./test/recurse/lol*.xml", NULL, NULL, NULL,
      0 },
    { "Parsing recursive test cases (no substitution)" ,
      recursiveDetectTest, "./test/recurse/lol*.xml", NULL, NULL, NULL,
      OPT_NO_SUBST },
    { "Parsing recursive test cases (SAX)" ,
      recursiveDetectTest, "./test/recurse/lol*.xml", NULL, NULL, NULL,
      OPT_SAX },
    { "Parsing recursive test cases (SAX, no substitution)" ,
      recursiveDetectTest, "./test/recurse/lol*.xml", NULL, NULL, NULL,
      OPT_SAX | OPT_NO_SUBST },
    { "Parsing non-recursive test cases" ,
      notRecursiveDetectTest, "./test/recurse/good*.xml", NULL, NULL, NULL,
      0 },
    { "Parsing non-recursive test cases (SAX)" ,
      notRecursiveDetectTest, "./test/recurse/good*.xml", NULL, NULL, NULL,
      OPT_SAX },
    { "Parsing non-recursive huge case" ,
      notRecursiveHugeTest, NULL, NULL, NULL, NULL,
      0 },
    { "Parsing non-recursive huge case (no substitution)" ,
      notRecursiveHugeTest, NULL, NULL, NULL, NULL,
      OPT_NO_SUBST },
    { "Parsing non-recursive huge case (SAX)" ,
      notRecursiveHugeTest, NULL, NULL, NULL, NULL,
      OPT_SAX },
    { "Parsing non-recursive huge case (SAX, no substitution)" ,
      notRecursiveHugeTest, NULL, NULL, NULL, NULL,
      OPT_SAX | OPT_NO_SUBST },
    { "Parsing non-recursive huge DTD case" ,
      hugeDtdTest, NULL, NULL, NULL, NULL,
      0 },
    {NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

/************************************************************************
 *									*
 *		The main code driving the tests				*
 *									*
 ************************************************************************/

static int
launchTests(testDescPtr tst) {
    int res = 0, err = 0;
    size_t i;
    char *result;
    char *error;
    int mem;

    if (tst == NULL) return(-1);
    if (tst->in != NULL) {
	glob_t globbuf;

	globbuf.gl_offs = 0;
	glob(tst->in, GLOB_DOOFFS, NULL, &globbuf);
	for (i = 0;i < globbuf.gl_pathc;i++) {
	    if (!checkTestFile(globbuf.gl_pathv[i]))
	        continue;
	    if (tst->suffix != NULL) {
		result = resultFilename(globbuf.gl_pathv[i], tst->out,
					tst->suffix);
		if (result == NULL) {
		    fprintf(stderr, "Out of memory !\n");
		    fatalError();
		}
	    } else {
	        result = NULL;
	    }
	    if (tst->err != NULL) {
		error = resultFilename(globbuf.gl_pathv[i], tst->out,
		                        tst->err);
		if (error == NULL) {
		    fprintf(stderr, "Out of memory !\n");
		    fatalError();
		}
	    } else {
	        error = NULL;
	    }
	    if ((result) &&(!checkTestFile(result))) {
	        fprintf(stderr, "Missing result file %s\n", result);
	    } else if ((error) &&(!checkTestFile(error))) {
	        fprintf(stderr, "Missing error file %s\n", error);
	    } else {
		mem = xmlMemUsed();
		extraMemoryFromResolver = 0;
		testErrorsSize = 0;
		testErrors[0] = 0;
		res = tst->func(globbuf.gl_pathv[i], result, error,
		                tst->options | XML_PARSE_COMPACT);
		xmlResetLastError();
		if (res != 0) {
		    fprintf(stderr, "File %s generated an error\n",
		            globbuf.gl_pathv[i]);
		    nb_errors++;
		    err++;
		}
		else if (xmlMemUsed() != mem) {
		    if ((xmlMemUsed() != mem) &&
		        (extraMemoryFromResolver == 0)) {
			fprintf(stderr, "File %s leaked %d bytes\n",
				globbuf.gl_pathv[i], xmlMemUsed() - mem);
			nb_leaks++;
			err++;
		    }
		}
		testErrorsSize = 0;
	    }
	    if (result)
		free(result);
	    if (error)
		free(error);
	}
	globfree(&globbuf);
    } else {
        testErrorsSize = 0;
	testErrors[0] = 0;
	extraMemoryFromResolver = 0;
        res = tst->func(NULL, NULL, NULL, tst->options);
	if (res != 0) {
	    nb_errors++;
	    err++;
	}
    }
    return(err);
}

static int verbose = 0;
static int tests_quiet = 0;

static int
runtest(int i) {
    int ret = 0, res;
    int old_errors, old_tests, old_leaks;

    old_errors = nb_errors;
    old_tests = nb_tests;
    old_leaks = nb_leaks;
    if ((tests_quiet == 0) && (testDescriptions[i].desc != NULL))
	printf("## %s\n", testDescriptions[i].desc);
    res = launchTests(&testDescriptions[i]);
    if (res != 0)
	ret++;
    if (verbose) {
	if ((nb_errors == old_errors) && (nb_leaks == old_leaks))
	    printf("Ran %d tests, no errors\n", nb_tests - old_tests);
	else
	    printf("Ran %d tests, %d errors, %d leaks\n",
		   nb_tests - old_tests,
		   nb_errors - old_errors,
		   nb_leaks - old_leaks);
    }
    return(ret);
}

int
main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED) {
    int i, a, ret = 0;
    int subset = 0;

    initializeLibxml2();

    for (a = 1; a < argc;a++) {
        if (!strcmp(argv[a], "-v"))
	    verbose = 1;
        else if (!strcmp(argv[a], "-quiet"))
	    tests_quiet = 1;
	else {
	    for (i = 0; testDescriptions[i].func != NULL; i++) {
	        if (strstr(testDescriptions[i].desc, argv[a])) {
		    ret += runtest(i);
		    subset++;
		}
	    }
	}
    }
    if (subset == 0) {
	for (i = 0; testDescriptions[i].func != NULL; i++) {
	    ret += runtest(i);
	}
    }
    if ((nb_errors == 0) && (nb_leaks == 0)) {
        ret = 0;
	printf("Total %d tests, no errors\n",
	       nb_tests);
    } else {
        ret = 1;
	printf("Total %d tests, %d errors, %d leaks\n",
	       nb_tests, nb_errors, nb_leaks);
    }
    xmlCleanupParser();

    return(ret);
}
