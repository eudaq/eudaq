/********************************************************************\

   Name:         mxml.c
   Created by:   Stefan Ritt
   Copyright 2000 + Stefan Ritt

   Contents:     Midas XML Library

   This is a simple implementation of XML functions for writing and
   reading XML files. For writing an XML file from scratch, following
   functions can be used:

   writer = mxml_open_file(file_name);
     mxml_start_element(writer, name);
     mxml_write_attribute(writer, name, value);
     mxml_write_value(writer, value);
     mxml_end_element(writer); 
     ...
   mxml_close_file(writer);

   To read an XML file, the function

   tree = mxml_parse_file(file_name, error, sizeof(error));

   is used. It parses the complete XML file and stores it in a
   hierarchical tree in memory. Nodes in that tree can be searched
   for with

   mxml_find_node(tree, xml_path);

   or

   mxml_find_nodes(tree, xml_path, &nodelist);

   which support a subset of the XPath specification. Another set of
   functions is availabe to retrieve attributes and values from nodes
   in the tree and for manipulating nodes, like replacing, adding and
   deleting nodes.
   
   
   This file is part of MIDAS XML Library.

   MIDAS XML Library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MIDAS XML Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MIDAS XML Library.  If not, see <http://www.gnu.org/licenses/>.

\********************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER

#include <windows.h>
#include <io.h>
#include <time.h>

#pragma warning( disable: 4996) /* disable "deprecated" warning */

#else

#define TRUE 1
#define FALSE 0

#ifndef O_TEXT
#define O_TEXT 0
#define O_BINARY 0
#endif

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#ifndef OS_VXWORKS
#include <sys/time.h>
#endif
#include <time.h>

#endif

#include "mxml.h"
#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

#define XML_INDENT "  "

#if defined(__GNUC__) && !defined(__MAKECINT__)
#   define MXML_GNUC_PRINTF( format_idx, arg_idx )          \
   __attribute__((format (printf, format_idx, arg_idx)))
#   define MXML_GNUC_SCANF( format_idx, arg_idx )           \
   __attribute__((format (scanf, format_idx, arg_idx)))
#   define MXML_GNUC_FORMAT( arg_idx )                      \
   __attribute__((format_arg (arg_idx)))
#else
#   define MXML_GNUC_PRINTF( format_idx, arg_idx )
#   define MXML_GNUC_SCANF( format_idx, arg_idx )
#   define MXML_GNUC_FORMAT( arg_idx )
#endif

static int mxml_suppress_date_flag = 0; /* suppress writing date at the top of file. */

/* local prototypes */
static PMXML_NODE read_error(PMXML_NODE root, const char *file_name, int line_number, char *error, int error_size, int *error_line, const char *format, ...) MXML_GNUC_PRINTF(7, 8);
static void mxml_encode(char *src, int size, int translate);
static void mxml_decode(char *str);
static int mxml_write_subtree(MXML_WRITER *writer, PMXML_NODE tree, int indent);
static int mxml_write_line(MXML_WRITER *writer, const char *line);
static int mxml_start_element1(MXML_WRITER *writer, const char *name, int indent);
static int mxml_add_resultnode(PMXML_NODE node, const char *xml_path, PMXML_NODE **nodelist, int *found);
static int mxml_find_nodes1(PMXML_NODE tree, const char *xml_path, PMXML_NODE **nodelist, int *found);
static void *mxml_malloc(size_t size);
static void *mxml_realloc(void *p, size_t size);
static void mxml_free(void *p);
static void mxml_deallocate(void);

/*------------------------------------------------------------------*/

static char *_encode_buffer = NULL;
static char *_data_enc = NULL;

/*------------------------------------------------------------------*/

void *mxml_malloc(size_t size)
{
   return malloc(size);
}

/*------------------------------------------------------------------*/

void *mxml_realloc(void *p, size_t size)
{
   return realloc(p, size);
}

/*------------------------------------------------------------------*/

void mxml_free(void *p)
{
   free(p);
}

/*------------------------------------------------------------------*/

void mxml_deallocate(void)
{
   if (_encode_buffer != NULL) {
      mxml_free(_encode_buffer);
      _encode_buffer = NULL;
   }
   if (_data_enc != NULL) {
      mxml_free(_data_enc);
      _data_enc = NULL;
   }
}

/*------------------------------------------------------------------*/

int mxml_write_line(MXML_WRITER *writer, const char *line)
{
   int len;
   
   len = (int)strlen(line);

   if (writer->buffer) {
      if (writer->buffer_len + len >= writer->buffer_size) {
         writer->buffer_size += 10000;
         writer->buffer = (char *)mxml_realloc(writer->buffer, writer->buffer_size);
      }
      strcpy(writer->buffer + writer->buffer_len, line);
      writer->buffer_len += len;
      return len;
   } else {
      return (int)write(writer->fh, line, len);
   }

   return 0;
}

/*------------------------------------------------------------------*/

/**
 * open a memory buffer and write XML header
 */
MXML_WRITER *mxml_open_buffer(void)
{
   char str[256], line[1000];
   time_t now;
   MXML_WRITER *writer;

   writer = (MXML_WRITER *)mxml_malloc(sizeof(MXML_WRITER));
   memset(writer, 0, sizeof(MXML_WRITER));
   writer->translate = 1;

   writer->buffer_size = 10000;
   writer->buffer = (char *)mxml_malloc(10000);
   writer->buffer[0] = 0;
   writer->buffer_len = 0;

   /* write XML header */
   strcpy(line, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
   mxml_write_line(writer, line);
   time(&now);
   strcpy(str, ctime(&now));
   str[24] = 0;
   sprintf(line, "<!-- created by MXML on %s -->\n", str);
   if (mxml_suppress_date_flag == 0)
      mxml_write_line(writer, line);

   /* initialize stack */
   writer->level = 0;
   writer->element_is_open = 0;

   return writer;
}

/*------------------------------------------------------------------*/

/**
 * suppress writing date at the top of file.
 */
void mxml_suppress_date(int suppress)
{
   mxml_suppress_date_flag = suppress;
}

/*------------------------------------------------------------------*/

/**
 * open a file and write XML header
 */
MXML_WRITER *mxml_open_file(const char *file_name) 
{
   char str[256], line[1000];
   time_t now;
   MXML_WRITER *writer;

   writer = (MXML_WRITER *)mxml_malloc(sizeof(MXML_WRITER));
   memset(writer, 0, sizeof(MXML_WRITER));
   writer->translate = 1;

   writer->fh = open(file_name, O_RDWR | O_CREAT | O_TRUNC | O_TEXT, 0644);

   if (writer->fh == -1) {
      sprintf(line, "Unable to open file \"%s\": ", file_name);
      perror(line);
      mxml_free(writer);
      return NULL;
   }

   /* write XML header */
   strcpy(line, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
   mxml_write_line(writer, line);
   time(&now);
   strcpy(str, ctime(&now));
   str[24] = 0;
   sprintf(line, "<!-- created by MXML on %s -->\n", str);
   if (mxml_suppress_date_flag == 0)
      mxml_write_line(writer, line);

   /* initialize stack */
   writer->level = 0;
   writer->element_is_open = 0;

   return writer;
}

/*------------------------------------------------------------------*/

/**
 * convert '<' '>' '&' '"' ''' into &xx;
 */
void mxml_encode(char *src, int size, int translate)
{
   char *ps, *pd;
   static int buffer_size = 1000;

   assert(size);

   if (_encode_buffer == NULL) {
      _encode_buffer = (char *) mxml_malloc(buffer_size);
      atexit(mxml_deallocate);
   }

   if (size > buffer_size) {
      _encode_buffer = (char *) mxml_realloc(_encode_buffer, size*2);
      buffer_size = size;
   }

   pd = _encode_buffer;
   for (ps = src ; *ps && (size_t)pd - (size_t)_encode_buffer < (size_t)(size-10) ; ps++) {

     if (translate) { /* tranlate "<", ">", "&", """, "'" */
         switch (*ps) {
         case '<':
            strcpy(pd, "&lt;");
            pd += 4;
            break;
         case '>':
            strcpy(pd, "&gt;");
            pd += 4;
            break;
         case '&':
            strcpy(pd, "&amp;");
            pd += 5;
            break;
         case '\"':
            strcpy(pd, "&quot;");
            pd += 6;
            break;
         case '\'':
            strcpy(pd, "&apos;");
            pd += 6;
            break;
         default:
            *pd++ = *ps;
         }
      } else {
       switch (*ps) { /* translate only illegal XML characters "<" and "&" */
         case '<':
            strcpy(pd, "&lt;");
            pd += 4;
            break;
         case '&':
            strcpy(pd, "&amp;");
            pd += 5;
            break;
         default:
            *pd++ = *ps;
         }
      }
   }
   *pd = 0;

   strlcpy(src, _encode_buffer, size);
}

/*------------------------------------------------------------------*/

/**
 * reverse of mxml_encode, strip leading or trailing '"'
 */
void mxml_decode(char *str)
{
   char *p;

   p = str;
   while ((p = strchr(p, '&')) != NULL) {
      if (strncmp(p, "&lt;", 4) == 0) {
         *(p++) = '<';
         memmove(p, p+3, strlen(p+3) + 1);
      }
      else if (strncmp(p, "&gt;", 4) == 0) {
         *(p++) = '>';
         memmove(p, p+3, strlen(p+3) + 1);
      }
      else if (strncmp(p, "&amp;", 5) == 0) {
         *(p++) = '&';
         memmove(p, p+4, strlen(p+4) + 1);
      }
      else if (strncmp(p, "&quot;", 6) == 0) {
         *(p++) = '\"';
         memmove(p, p+5, strlen(p+5) + 1);
      }
      else if (strncmp(p, "&apos;", 6) == 0) {
         *(p++) = '\'';
         memmove(p, p+5, strlen(p+5) + 1);
      }
      else {
         p++; // skip unknown entity
      }
   }
/*   if (str[0] == '\"' && str[strlen(str)-1] == '\"') {
      memmove(str, str+1, strlen(str+1) + 1);
      str[strlen(str)-1] = 0;
   }*/
}

/*------------------------------------------------------------------*/

/**
 * set translation of <,>,",',&, on/off in writer
 */
int mxml_set_translate(MXML_WRITER *writer, int flag)
{
   int old_flag;

   old_flag = writer->translate;
   writer->translate = flag;
   return old_flag;
}
/*------------------------------------------------------------------*/

/**
 * start a new XML element, must be followed by mxml_end_elemnt
 */
int mxml_start_element1(MXML_WRITER *writer, const char *name, int indent)
{
   int i;
   char line[1000], name_enc[1000];

   if (writer->element_is_open) {
      mxml_write_line(writer, ">\n");
      writer->element_is_open = FALSE;
   }

   line[0] = 0;
   if (indent)
      for (i=0 ; i<writer->level ; i++)
         strlcat(line, XML_INDENT, sizeof(line));
   strlcat(line, "<", sizeof(line));
   strlcpy(name_enc, name, sizeof(name_enc));
   mxml_encode(name_enc, sizeof(name_enc), writer->translate);
   strlcat(line, name_enc, sizeof(line));

   /* put element on stack */
   if (writer->level == 0)
      writer->stack = (char **)mxml_malloc(sizeof(char *));
   else
      writer->stack = (char **)mxml_realloc(writer->stack, sizeof(char *)*(writer->level+1));
   
   writer->stack[writer->level] = (char *) mxml_malloc(strlen(name_enc)+1);
   strcpy(writer->stack[writer->level], name_enc);
   writer->level++;
   writer->element_is_open = TRUE;
   writer->data_was_written = FALSE;

   return mxml_write_line(writer, line) == (int)strlen(line);
}

/*------------------------------------------------------------------*/

int mxml_start_element(MXML_WRITER *writer, const char *name)
{
   return mxml_start_element1(writer, name, TRUE);
}

/*------------------------------------------------------------------*/

int mxml_start_element_noindent(MXML_WRITER *writer, const char *name)
{
   return mxml_start_element1(writer, name, FALSE);
}

/*------------------------------------------------------------------*/

/**
 * close an open XML element
 */
int mxml_end_element(MXML_WRITER *writer)
{
   int i;
   char line[1000];

   if (writer->level == 0)
      return 0;
   
   writer->level--;

   if (writer->element_is_open) {
      writer->element_is_open = FALSE;
      mxml_free(writer->stack[writer->level]);
      if (writer->level == 0)
         mxml_free(writer->stack);
      strcpy(line, "/>\n");
      return mxml_write_line(writer, line) == (int)strlen(line);
   }

   line[0] = 0;
   if (!writer->data_was_written) {
      for (i=0 ; i<writer->level ; i++)
         strlcat(line, XML_INDENT, sizeof(line));
   }

   strlcat(line, "</", sizeof(line));
   strlcat(line, writer->stack[writer->level], sizeof(line));
   mxml_free(writer->stack[writer->level]);
   if (writer->level == 0)
      mxml_free(writer->stack);
   strlcat(line, ">\n", sizeof(line));
   writer->data_was_written = FALSE;

   return mxml_write_line(writer, line) == (int)strlen(line);
}

/*------------------------------------------------------------------*/

/**
 * write an attribute to the currently open XML element
 */
int mxml_write_attribute(MXML_WRITER *writer, const char *name, const char *value)
{
   char name_enc[4096], val_enc[4096], line[8192];

   if (!writer->element_is_open)
      return FALSE;

   strcpy(name_enc, name);
   mxml_encode(name_enc, sizeof(name_enc), writer->translate);
   strcpy(val_enc, value);
   mxml_encode(val_enc, sizeof(val_enc), writer->translate);

   sprintf(line, " %s=\"%s\"", name_enc, val_enc);

   return mxml_write_line(writer, line) == (int)strlen(line);
}

/*------------------------------------------------------------------*/

/**
 * write value of an XML element, like <[name]>[value]</[name]>
 */
int mxml_write_value(MXML_WRITER *writer, const char *data)
{
   static int data_size = 0;

   if (!writer->element_is_open)
      return FALSE;

   if (mxml_write_line(writer, ">") != 1)
      return FALSE;
   writer->element_is_open = FALSE;
   writer->data_was_written = TRUE;

   if (data_size == 0) {
      _data_enc = (char *)mxml_malloc(1000);
      data_size = 1000;
   } else if ((int)strlen(data)*2+1000 > data_size) {
      data_size = 1000+(int)strlen(data)*2;
      _data_enc = (char *)mxml_realloc(_data_enc, data_size);
   }

   strcpy(_data_enc, data);
   mxml_encode(_data_enc, data_size, writer->translate);
   return mxml_write_line(writer, _data_enc) == (int)strlen(_data_enc);
}

/*------------------------------------------------------------------*/

/**
 * write empty line
 */
int mxml_write_empty_line(MXML_WRITER *writer)
{
   if (writer->element_is_open) {
      mxml_write_line(writer, ">\n");
      writer->element_is_open = FALSE;
   }

   if (mxml_write_line(writer, "\n") != 1)
      return FALSE;

   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * write a comment to an XML file, enclosed in "<!--" and "-->"
 */
int mxml_write_comment(MXML_WRITER *writer, const char *string)
{
   int  i;
   char line[1000];

   if (writer->element_is_open) {
      mxml_write_line(writer, ">\n");
      writer->element_is_open = FALSE;
   }

   line[0] = 0;
   for (i=0 ; i<writer->level ; i++)
      strlcat(line, XML_INDENT, sizeof(line));

   strlcat(line, "<!-- ", sizeof(line));
   strlcat(line, string, sizeof(line));
   strlcat(line, " -->\n", sizeof(line));
   if (mxml_write_line(writer, line) != (int)strlen(line))
      return FALSE;

   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * shortcut to write an element with a value but without attribute
 */
int mxml_write_element(MXML_WRITER *writer, const char *name, const char *value)
{
   int i;

   i = mxml_start_element(writer, name);
   i += mxml_write_value(writer, value);
   i += mxml_end_element(writer);
   return i;
}

/*------------------------------------------------------------------*/

/**
 * close a file opened with mxml_open_writer
 */
char *mxml_close_buffer(MXML_WRITER *writer)
{
   int i;
   char *p;

   if (writer->element_is_open) {
      writer->element_is_open = FALSE;
      if (mxml_write_line(writer, ">\n") != 2)
         return NULL;
   }

   /* close remaining open levels */
   for (i = 0 ; i<writer->level ; i++)
      mxml_end_element(writer);

   p = writer->buffer;
   mxml_free(writer);
   return p;
}

/*------------------------------------------------------------------*/

/**
 * close a file opened with mxml_open_writer
 */
int mxml_close_file(MXML_WRITER *writer)
{
   int i;

   if (writer->element_is_open) {
      writer->element_is_open = FALSE;
      if (mxml_write_line(writer, ">\n") != 2)
         return 0;
   }

   /* close remaining open levels */
   for (i = 0 ; i<writer->level ; i++)
      mxml_end_element(writer);

   close(writer->fh);
   mxml_free(writer);
   return 1;
}

/*------------------------------------------------------------------*/

/**
 * create root node of an XML tree
 */
PMXML_NODE mxml_create_root_node(void)
{
   PMXML_NODE root;

   root = (PMXML_NODE)calloc(sizeof(MXML_NODE), 1);
   strcpy(root->name, "root");
   root->node_type = DOCUMENT_NODE;

   return root;
}

/*------------------------------------------------------------------*/

/**
 * add a subnode (child) to an existing parent node as a specific position
 */
PMXML_NODE mxml_add_special_node_at(PMXML_NODE parent, int node_type, const char *node_name, const char *value, int idx)
{
   PMXML_NODE pnode, pchild;
   int i, j;

   assert(parent);
   if (parent->n_children == 0)
      parent->child = (PMXML_NODE)mxml_malloc(sizeof(MXML_NODE));
   else
      parent->child = (PMXML_NODE)mxml_realloc(parent->child, sizeof(MXML_NODE)*(parent->n_children+1));
   assert(parent->child);

   /* move following nodes one down */
   if (idx < parent->n_children) 
      for (i=parent->n_children ; i > idx ; i--)
         memcpy(&parent->child[i], &parent->child[i-1], sizeof(MXML_NODE));

   /* correct parent pointer for children */
   for (i=0 ; i<parent->n_children ; i++) {
      pchild = parent->child+i;
      for (j=0 ; j<pchild->n_children ; j++)
         pchild->child[j].parent = pchild;
   }

   /* initialize new node */
   pnode = &parent->child[idx];
   memset(pnode, 0, sizeof(MXML_NODE));
   strlcpy(pnode->name, node_name, sizeof(pnode->name));
   pnode->node_type = node_type;
   pnode->parent = parent;
   
   parent->n_children++;

   if (value && *value) {
      pnode->value = (char *)mxml_malloc(strlen(value)+1);
      assert(pnode->value);
      strcpy(pnode->value, value);
   }

   return pnode;
}

/*------------------------------------------------------------------*/

/**
 * add a subnode (child) to an existing parent node at the end
 */
PMXML_NODE mxml_add_special_node(PMXML_NODE parent, int node_type, const char *node_name, const char *value)
{
   return mxml_add_special_node_at(parent, node_type, node_name, value, parent->n_children);
}

/*------------------------------------------------------------------*/

/**
 * write value of an XML element, like <[name]>[value]</[name]>
 */
PMXML_NODE mxml_add_node(PMXML_NODE parent, const char *node_name, const char *value)
{
   return mxml_add_special_node_at(parent, ELEMENT_NODE, node_name, value, parent->n_children);
}

/*------------------------------------------------------------------*/

/**
 * add a subnode (child) to an existing parent node at the end
 */
PMXML_NODE mxml_add_node_at(PMXML_NODE parent, const char *node_name, const char *value, int idx)
{
   return mxml_add_special_node_at(parent, ELEMENT_NODE, node_name, value, idx);
}

/*------------------------------------------------------------------*/

/**
 * add a whole node tree to an existing parent node at a specific position
 */
int mxml_add_tree_at(PMXML_NODE parent, PMXML_NODE tree, int idx)
{
   PMXML_NODE pchild;
   int i, j, k;

   assert(parent);
   assert(tree);
   if (parent->n_children == 0)
      parent->child = (PMXML_NODE)mxml_malloc(sizeof(MXML_NODE));
   else {
      pchild = parent->child;
      parent->child = (PMXML_NODE)mxml_realloc(parent->child, sizeof(MXML_NODE)*(parent->n_children+1));

      if (parent->child != pchild) {
         /* correct parent pointer for children */
         for (i=0 ; i<parent->n_children ; i++) {
            pchild = parent->child+i;
            for (j=0 ; j<pchild->n_children ; j++)
               pchild->child[j].parent = pchild;
         }
      }
   }
   assert(parent->child);

   if (idx < parent->n_children) 
      for (i=parent->n_children ; i > idx ; i--) {
         /* move following nodes one down */
         memcpy(&parent->child[i], &parent->child[i-1], sizeof(MXML_NODE));

         /* correct parent pointer for children */
         for (j=0 ; j<parent->n_children ; j++) {
            pchild = parent->child+j;
            for (k=0 ; k<pchild->n_children ; k++)
               pchild->child[k].parent = pchild;
         }
      }

   /* initialize new node */
   memcpy(parent->child+idx, tree, sizeof(MXML_NODE));
   parent->n_children++;
   parent->child[idx].parent = parent;

   /* correct parent pointer for children */
   for (i=0 ; i<parent->n_children ; i++) {
      pchild = parent->child+i;
      for (j=0 ; j<pchild->n_children ; j++)
         pchild->child[j].parent = pchild;
   }

   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * add a whole node tree to an existing parent node at the end
 */
int mxml_add_tree(PMXML_NODE parent, PMXML_NODE tree)
{
   return mxml_add_tree_at(parent, tree, parent->n_children);
}

/*------------------------------------------------------------------*/

/**
 * add an attribute to an existing node
 */
int mxml_add_attribute(PMXML_NODE pnode, const char *attrib_name, const char *attrib_value)
{
   if (pnode->n_attributes == 0) {
      pnode->attribute_name  = (char*)mxml_malloc(MXML_NAME_LENGTH);
      pnode->attribute_value = (char**)mxml_malloc(sizeof(char *));
   } else {
      pnode->attribute_name  = (char*)mxml_realloc(pnode->attribute_name,  MXML_NAME_LENGTH*(pnode->n_attributes+1));
      pnode->attribute_value = (char**)mxml_realloc(pnode->attribute_value, sizeof(char *)*(pnode->n_attributes+1));
   }

   strlcpy(pnode->attribute_name+pnode->n_attributes*MXML_NAME_LENGTH, attrib_name, MXML_NAME_LENGTH);
   pnode->attribute_value[pnode->n_attributes] = (char *)mxml_malloc(strlen(attrib_value)+1);
   strcpy(pnode->attribute_value[pnode->n_attributes], attrib_value);
   pnode->n_attributes++;

   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * return number of subnodes (children) of a node
 */
int mxml_get_number_of_children(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->n_children;
}

/*------------------------------------------------------------------*/

/**
 * return number of subnodes (children) of a node
 */
PMXML_NODE mxml_subnode(PMXML_NODE pnode, int idx)
{
   assert(pnode);
   if (idx < pnode->n_children)
      return &pnode->child[idx];
   return NULL;
}

/*------------------------------------------------------------------*/


int mxml_find_nodes1(PMXML_NODE tree, const char *xml_path, PMXML_NODE **nodelist, int *found);

int mxml_add_resultnode(PMXML_NODE node, const char *xml_path, PMXML_NODE **nodelist, int *found)
{
   /* if at end of path, add this node */
   if (*xml_path == 0) {
      if (*found == 0)
         *nodelist = (PMXML_NODE *)mxml_malloc(sizeof(PMXML_NODE));
      else
         *nodelist = (PMXML_NODE *)mxml_realloc(*nodelist, sizeof(PMXML_NODE)*(*found + 1));

      (*nodelist)[*found] = node;
      (*found)++;
   } else {
      /* if not at end of path, branch into subtree */
      return mxml_find_nodes1(node, xml_path+1, nodelist, found);
   }

   return 1;
}

/*------------------------------------------------------------------*/

/**
   Return list of XML nodes with a subset of XPATH specifications.
   Following elemets are possible

   /<node>/<node>/..../<node>          Find a node in the tree hierarchy
   /<node>[idx]                        Find child #[idx] of node (index starts from 1)
   /<node>[idx]/<node>                 Find subnode of the above
   /<node>[<subnode>=<value>]          Find a node which has a specific subnode
   /<node>[<subnode>=<value>]/<node>   Find subnode of the above
   /<node>[@<attrib>=<value>]/<node>   Find a node which has a specific attribute
*/
int mxml_find_nodes1(PMXML_NODE tree, const char *xml_path, PMXML_NODE **nodelist, int *found)
{
   PMXML_NODE pnode;
   const char *p1,*p2;
   char *p3, node_name[256], condition[256];
   char cond_name[MXML_MAX_CONDITION][256], cond_value[MXML_MAX_CONDITION][256];
   int  cond_type[MXML_MAX_CONDITION];
   int i, j, k, idx, num_cond;
   int cond_satisfied,cond_index;
   size_t len;

   p1 = xml_path;
   pnode = tree;

   /* skip leading '/' */
   if (*p1 && *p1 == '/')
      p1++;

   do {
      p2 = p1;
      while (*p2 && *p2 != '/' && *p2 != '[')
         p2++;
      len = (size_t)p2 - (size_t)p1;
      if (len >= sizeof(node_name))
         return 0;

      memcpy(node_name, p1, len);
      node_name[len] = 0;
      idx = 0;
      num_cond = 0;
      while (*p2 == '[') {
         cond_name[num_cond][0] = cond_value[num_cond][0] = cond_type[num_cond] = 0;
         p2++;
         if (isdigit(*p2)) {
            /* evaluate [idx] */
            idx = atoi(p2);
            p2 = strchr(p2, ']');
            if (p2 == NULL)
               return 0;
            p2++;
         } else {
            /* evaluate [<@attrib>/<subnode>=<value>] */
            while (*p2 && isspace((unsigned char)*p2))
               p2++;
            strlcpy(condition, p2, sizeof(condition));
            if (strchr(condition, ']'))
               *strchr(condition, ']') = 0;
            else
               return 0;
            p2 = strchr(p2, ']')+1;
            if ((p3 = strchr(condition, '=')) != NULL) {
               if (condition[0] == '@') {
                  cond_type[num_cond] = 1;
                  strlcpy(cond_name[num_cond], &condition[1], sizeof(cond_name[num_cond]));
               } else {
                  strlcpy(cond_name[num_cond], condition, sizeof(cond_name[num_cond]));
               }

               *strchr(cond_name[num_cond], '=') = 0;
               while (cond_name[num_cond][0] && isspace(cond_name[num_cond][strlen(cond_name[num_cond])-1]))
                  cond_name[num_cond][strlen(cond_name[num_cond])-1] = 0;

               p3++;
               while (*p3 && isspace(*p3))
                  p3++;
               if (*p3 == '\"') {
                  strlcpy(cond_value[num_cond], p3+1, sizeof(cond_value[num_cond]));
                  while (cond_value[num_cond][0] && isspace(cond_value[num_cond][strlen(cond_value[num_cond])-1]))
                     cond_value[num_cond][strlen(cond_value[num_cond])-1] = 0;
                  if (cond_value[num_cond][0] && cond_value[num_cond][strlen(cond_value[num_cond])-1] == '\"')
                     cond_value[num_cond][strlen(cond_value[num_cond])-1] = 0;
               } else if (*p3 == '\'') {
                  strlcpy(cond_value[num_cond], p3+1, sizeof(cond_value[num_cond]));
                  while (cond_value[num_cond][0] && isspace(cond_value[num_cond][strlen(cond_value[num_cond])-1]))
                     cond_value[num_cond][strlen(cond_value[num_cond])-1] = 0;
                  if (cond_value[num_cond][0] && cond_value[num_cond][strlen(cond_value[num_cond])-1] == '\'')
                     cond_value[num_cond][strlen(cond_value[num_cond])-1] = 0;
               } else {
                  strlcpy(cond_value[num_cond], p3, sizeof(cond_value[num_cond]));
                  while (cond_value[num_cond][0] && isspace(cond_value[num_cond][strlen(cond_value[num_cond])-1]))
                     cond_value[num_cond][strlen(cond_value[num_cond])-1] = 0;
               }
               num_cond++;
            }
         }
      }

      cond_index = 0;
      for (i=j=0 ; i<pnode->n_children ; i++) {
         if (num_cond) {
            cond_satisfied = 0;
            for (k=0;k<num_cond;k++) {
               if (cond_type[k]) {
                  /* search node with attribute */
                  if (strcmp(pnode->child[i].name, node_name) == 0)
                     if (mxml_get_attribute(pnode->child+i, cond_name[k]) &&
                        strcmp(mxml_get_attribute(pnode->child+i, cond_name[k]), cond_value[k]) == 0)
                        cond_satisfied++;
               }
               else {
                  /* search subnode */
                  for (j=0 ; j<pnode->child[i].n_children ; j++)
                     if (strcmp(pnode->child[i].child[j].name, cond_name[k]) == 0)
                        if (strcmp(pnode->child[i].child[j].value, cond_value[k]) == 0)
                           cond_satisfied++;
               }
            }
            if (cond_satisfied==num_cond) {
               cond_index++;
               if (idx == 0 || cond_index == idx) {
                  if (!mxml_add_resultnode(pnode->child+i, p2, nodelist, found))
                     return 0;
               }
            }
         } else {
            if (strcmp(pnode->child[i].name, node_name) == 0)
               if (idx == 0 || ++j == idx)
                  if (!mxml_add_resultnode(pnode->child+i, p2, nodelist, found))
                     return 0;
         }
      }

      if (i == pnode->n_children)
         return 1;

      pnode = &pnode->child[i];
      p1 = p2;
      if (*p1 == '/')
         p1++;

   } while (*p2);

   return 1;
}

/*------------------------------------------------------------------*/

int mxml_find_nodes(PMXML_NODE tree, const char *xml_path, PMXML_NODE **nodelist)
{
   int status, found = 0;
   
   status = mxml_find_nodes1(tree, xml_path, nodelist, &found);

   if (status == 0)
      return -1;

   return found;
}

/*------------------------------------------------------------------*/

/**
 *  Search for a specific XML node with a subset of XPATH specifications.
 *  Return first found node. For syntax see mxml_find_nodes()
 */
PMXML_NODE mxml_find_node(PMXML_NODE tree, const char *xml_path)
{
   PMXML_NODE *node, pnode;
   int n;

   n = mxml_find_nodes(tree, xml_path, &node);
   if (n > 0) {
      pnode = node[0];
      mxml_free(node);
   } else 
      pnode = NULL;

   return pnode;
}

/*------------------------------------------------------------------*/

PMXML_NODE mxml_get_parent(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->parent;
}

/*------------------------------------------------------------------*/

char *mxml_get_name(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->name;
}

/*------------------------------------------------------------------*/

char *mxml_get_value(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->value;
}

/*------------------------------------------------------------------*/

int mxml_get_line_number_start(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->line_number_start;
}

/*------------------------------------------------------------------*/

int mxml_get_line_number_end(PMXML_NODE pnode)
{
   assert(pnode);
   return pnode->line_number_end;
}

/*------------------------------------------------------------------*/

char *mxml_get_attribute(PMXML_NODE pnode, const char *name)
{
   int i;

   assert(pnode);
   for (i=0 ; i<pnode->n_attributes ; i++) 
      if (strcmp(pnode->attribute_name+i*MXML_NAME_LENGTH, name) == 0)
         return pnode->attribute_value[i];

   return NULL;
}

/*------------------------------------------------------------------*/

int mxml_replace_node_name(PMXML_NODE pnode, const char *name)
{
   strlcpy(pnode->name, name, sizeof(pnode->name));
   return TRUE;
}

/*------------------------------------------------------------------*/

int mxml_replace_node_value(PMXML_NODE pnode, const char *value)
{
   if (pnode->value)
      pnode->value = (char *)mxml_realloc(pnode->value, strlen(value)+1);
   else if (value)
      pnode->value = (char *)mxml_malloc(strlen(value)+1);
   else
      pnode->value = NULL;
   
   if (value)
      strcpy(pnode->value, value);

   return TRUE;
}

/*------------------------------------------------------------------*/

/**
   replace value os a subnode, like

   <parent>
     <child>value</child>
   </parent>

   if pnode=parent, and "name"="child", then "value" gets replaced
*/
int mxml_replace_subvalue(PMXML_NODE pnode, const char *name, const char *value)
{
   int i;

   for (i=0 ; i<pnode->n_children ; i++) 
      if (strcmp(pnode->child[i].name, name) == 0)
         break;

   if (i == pnode->n_children)
      return FALSE;

   return mxml_replace_node_value(&pnode->child[i], value);
}

/*------------------------------------------------------------------*/

/**
 * change the name of an attribute, keep its value
 */
int mxml_replace_attribute_name(PMXML_NODE pnode, const char *old_name, const char *new_name)
{
   int i;

   for (i=0 ; i<pnode->n_attributes ; i++) 
      if (strcmp(pnode->attribute_name+i*MXML_NAME_LENGTH, old_name) == 0)
         break;

   if (i == pnode->n_attributes)
      return FALSE;

   strlcpy(pnode->attribute_name+i*MXML_NAME_LENGTH, new_name, MXML_NAME_LENGTH);
   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * change the value of an attribute
 */
int mxml_replace_attribute_value(PMXML_NODE pnode, const char *attrib_name, const char *attrib_value)
{
   int i;

   for (i=0 ; i<pnode->n_attributes ; i++) 
      if (strcmp(pnode->attribute_name+i*MXML_NAME_LENGTH, attrib_name) == 0)
         break;

   if (i == pnode->n_attributes)
      return FALSE;

   pnode->attribute_value[i] = (char *)mxml_realloc(pnode->attribute_value[i], strlen(attrib_value)+1);
   strcpy(pnode->attribute_value[i], attrib_value);
   return TRUE;
}

/*------------------------------------------------------------------*/

/**
 * free memory of a node and remove it from the parent's child list
 */
int mxml_delete_node(PMXML_NODE pnode)
{
   PMXML_NODE parent;
   int i, j;

   /* remove node from parent's list */
   parent = pnode->parent;

   if (parent) {
      for (i=0 ; i<parent->n_children ; i++)
         if (&parent->child[i] == pnode)
            break;

      /* free allocated node memory recursively */
      mxml_free_tree(pnode);

      if (i < parent->n_children) {
         for (j=i ; j<parent->n_children-1 ; j++)
            memcpy(&parent->child[j], &parent->child[j+1], sizeof(MXML_NODE));
         parent->n_children--;
         if (parent->n_children)
            parent->child = (PMXML_NODE)mxml_realloc(parent->child, sizeof(MXML_NODE)*(parent->n_children));
         else
            mxml_free(parent->child);
      }
   } else 
      mxml_free_tree(pnode);

   return TRUE;
}

/*------------------------------------------------------------------*/

int mxml_delete_attribute(PMXML_NODE pnode, const char *attrib_name)
{
   int i, j;

   for (i=0 ; i<pnode->n_attributes ; i++) 
      if (strcmp(pnode->attribute_name+i*MXML_NAME_LENGTH, attrib_name) == 0)
         break;

   if (i == pnode->n_attributes)
      return FALSE;

   mxml_free(pnode->attribute_value[i]);
   for (j=i ; j<pnode->n_attributes-1 ; j++) {
      strcpy(pnode->attribute_name+j*MXML_NAME_LENGTH, pnode->attribute_name+(j+1)*MXML_NAME_LENGTH);
      pnode->attribute_value[j] = pnode->attribute_value[j+1];
   }

   if (pnode->n_attributes > 0) {
      pnode->attribute_name  = (char *)mxml_realloc(pnode->attribute_name,  MXML_NAME_LENGTH*(pnode->n_attributes-1));
      pnode->attribute_value = (char **)mxml_realloc(pnode->attribute_value, sizeof(char *)*(pnode->n_attributes-1));
   } else {
      mxml_free(pnode->attribute_name);
      mxml_free(pnode->attribute_value);
   }

   return TRUE;
}

/*------------------------------------------------------------------*/

#define HERE root, file_name, line_number, error, error_size, error_line

/**
 * used inside mxml_parse_file for reporting errors
 */
PMXML_NODE read_error(PMXML_NODE root, const char *file_name, int line_number, char *error, int error_size, int *error_line, const char *format, ...)
{
   char *msg, str[1000];
   va_list argptr;

   if (file_name && file_name[0])
      sprintf(str, "XML read error in file \"%s\", line %d: ", file_name, line_number);
   else
      sprintf(str, "XML read error, line %d: ", line_number);
   msg = (char *)mxml_malloc(error_size);
   if (error)
      strlcpy(error, str, error_size);

   va_start(argptr, format);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

   if (error)
      strlcat(error, str, error_size);
   if (error_line)
      *error_line = line_number;
   
   mxml_free(msg);
   mxml_free_tree(root);

   return NULL;
}

/*------------------------------------------------------------------*/

/**
 * Parse a XML buffer and convert it into a tree of MXML_NODE's.
 * Return NULL in case of an error, return error description.
 * Optional file_name is used for error reporting if called from mxml_parse_file()
 */
PMXML_NODE mxml_parse_buffer(const char *buf, char *error, int error_size, int *error_line)
{
   char node_name[256], attrib_name[256], attrib_value[1000], quote;
   const char *p, *pv;
   int i,j, line_number;
   PMXML_NODE root, ptree, pnew;
   int end_element;
   size_t len;
   char *file_name = NULL; /* dummy for 'HERE' */

   p = buf;
   line_number = 1;

   root = mxml_create_root_node();
   ptree = root;

   /* parse file contents */
   do {
      if (*p == '<') {

         end_element = FALSE;

         /* found new element */
         p++;
         while (*p && isspace(*p)) {
            if (*p == '\n')
               line_number++;
            p++;
         }
         if (!*p)
            return read_error(HERE, "Unexpected end of file");

         if (strncmp(p, "!--", 3) == 0) {
            
            /* found comment */

            pnew = mxml_add_special_node(ptree, COMMENT_NODE, "Comment", NULL);
            pnew->line_number_start = line_number;
            pv = p+3;
            while (*pv == ' ')
               pv++;

            p += 3;
            if (strstr(p, "-->") == NULL)
               return read_error(HERE, "Unterminated comment");
            
            while (strncmp(p, "-->", 3) != 0) {
               if (*p == '\n')
                  line_number++;
               p++;
            }

            len = (size_t)p - (size_t)pv;
            pnew->value = (char *)mxml_malloc(len+1);
            memcpy(pnew->value, pv, len);
            pnew->value[len] = 0;
            pnew->line_number_end = line_number;
            mxml_decode(pnew->value);

            p += 3;

         } else if (*p == '?') {

            /* found ?...? element */
            pnew = mxml_add_special_node(ptree, PROCESSING_INSTRUCTION_NODE, "PI", NULL);
            pnew->line_number_start = line_number;
            pv = p+1;

            p++;
            if (strstr(p, "?>") == NULL)
               return read_error(HERE, "Unterminated ?...? element");
            
            while (strncmp(p, "?>", 2) != 0) {
               if (*p == '\n')
                  line_number++;
               p++;
            }

            len = (size_t)p - (size_t)pv;
            pnew->value = (char *)mxml_malloc(len+1);
            memcpy(pnew->value, pv, len);
            pnew->value[len] = 0;
            pnew->line_number_end = line_number;
            mxml_decode(pnew->value);

            p += 2;

         } else if (strncmp(p, "!DOCTYPE", 8) == 0 ) {

            /* found !DOCTYPE element , skip it */
            p += 8;
            if (strstr(p, ">") == NULL)
               return read_error(HERE, "Unterminated !DOCTYPE element");

            j = 0;
            while (*p && (*p != '>' || j > 0)) {
               if (*p == '\n')
                  line_number++;
               else if (*p == '<')
                  j++;
               else if (*p == '>')
                  j--;
               p++;
            }
            if (!*p)
               return read_error(HERE, "Unexpected end of file");

            p++;

         } else {
            
            /* found normal element */
            if (*p == '/') {
               end_element = TRUE;
               p++;
               while (*p && isspace((unsigned char)*p)) {
                  if (*p == '\n')
                     line_number++;
                  p++;
               }
               if (!*p)
                  return read_error(HERE, "Unexpected end of file");
            }

            /* extract node name */
            i = 0;
            node_name[i] = 0;
            while (*p && !isspace((unsigned char)*p) && *p != '/' && *p != '>' && *p != '<')
               node_name[i++] = *p++;
            node_name[i] = 0;
            if (!*p)
               return read_error(HERE, "Unexpected end of file");
            if (*p == '<')
               return read_error(HERE, "Unexpected \'<\' inside element \"%s\"", node_name);

            mxml_decode(node_name);

            if (end_element) {

               if (!ptree)
                  return read_error(HERE, "Found unexpected </%s>", node_name);

               /* close previously opened element */
               if (strcmp(ptree->name, node_name) != 0)
                  return read_error(HERE, "Found </%s>, expected </%s>", node_name, ptree->name);
               ptree->line_number_end = line_number;
               
               /* go up one level on the tree */
               ptree = ptree->parent;

            } else {
            
               if (ptree == NULL)
                  return read_error(HERE, "Unexpected second top level node");

               /* allocate new element structure in parent tree */
               pnew = mxml_add_node(ptree, node_name, NULL);
               pnew->line_number_start = line_number;
               pnew->line_number_end = line_number;

               while (*p && isspace((unsigned char)*p)) {
                  if (*p == '\n')
                     line_number++;
                  p++;
               }
               if (!*p)
                  return read_error(HERE, "Unexpected end of file");

               while (*p != '>' && *p != '/') {

                  /* found attribute */
                  pv = p;
                  while (*pv && !isspace((unsigned char)*pv) && *pv != '=' && *pv != '<' && *pv != '>')
                     pv++;
                  if (!*pv)
                     return read_error(HERE, "Unexpected end of file");
                  if (*pv == '<' || *pv == '>')
                     return read_error(HERE, "Unexpected \'%c\' inside element \"%s\"", *pv, node_name);

                  /* extract attribute name */
                  len = (size_t)pv - (size_t)p;
                  if (len > sizeof(attrib_name)-1)
                     len = sizeof(attrib_name)-1;
                  memcpy(attrib_name, p, len);
                  attrib_name[len] = 0;
                  mxml_decode(attrib_name);

                  p = pv;
                  while (*p && isspace((unsigned char)*p)) {
                     if (*p == '\n')
                        line_number++;
                     p++;
                  }
                  if (!*p)
                     return read_error(HERE, "Unexpected end of file");
                  if (*p != '=')
                     return read_error(HERE, "Expect \"=\" here");

                  p++;
                  while (*p && isspace((unsigned char)*p)) {
                     if (*p == '\n')
                        line_number++;
                     p++;
                  }
                  if (!*p)
                     return read_error(HERE, "Unexpected end of file");
                  if (*p != '\"' && *p != '\'')
                     return read_error(HERE, "Expect \" or \' here");
                  quote = *p;
                  p++;

                  /* extract attribute value */
                  pv = p;
                  while (*pv && *pv != quote)
                     pv++;
                  if (!*pv)
                     return read_error(HERE, "Unexpected end of file");

                  len = (size_t)pv - (size_t)p;
                  if (len > sizeof(attrib_value)-1)
                     len = sizeof(attrib_value)-1;
                  memcpy(attrib_value, p, len);
                  attrib_value[len] = 0;
                  mxml_decode(attrib_value);

                  /* add attribute to current node */
                  mxml_add_attribute(pnew, attrib_name, attrib_value);

                  p = pv+1;
                  while (*p && isspace((unsigned char)*p)) {
                     if (*p == '\n')
                        line_number++;
                     p++;
                  }
                  if (!*p)
                     return read_error(HERE, "Unexpected end of file");
               }

               if (*p == '/') {

                  /* found empty node, like <node/>, just skip closing bracket */
                  p++;

                  while (*p && isspace((unsigned char)*p)) {
                     if (*p == '\n')
                        line_number++;
                     p++;
                  }
                  if (!*p)
                     return read_error(HERE, "Unexpected end of file");
                  if (*p != '>')
                     return read_error(HERE, "Expected \">\" after \"/\"");
                  p++;
               }

               if (*p == '>') {

                  p++;

                  /* check if we have sub-element or value */
                  pv = p;
                  while (*pv && isspace((unsigned char)*pv)) {
                     if (*pv == '\n')
                        line_number++;
                     pv++;
                  }
                  if (!*pv)
                     return read_error(HERE, "Unexpected end of file");

                  if (*pv == '<' && *(pv+1) != '/') {

                     /* start new subtree */
                     ptree = pnew;
                     p = pv;

                  } else {

                     /* extract value */
                     while (*pv && *pv != '<') {
                        if (*pv == '\n')
                           line_number++;
                        pv++;
                     }
                     if (!*pv)
                        return read_error(HERE, "Unexpected end of file");

                     len = (size_t)pv - (size_t)p;
                     pnew->value = (char *)mxml_malloc(len+1);
                     memcpy(pnew->value, p, len);
                     pnew->value[len] = 0;
                     mxml_decode(pnew->value);
                     p = pv;

                     ptree = pnew;
                  }
               }
            }
         }
      }

      /* go to next element */
      while (*p && *p != '<') {
         if (*p == '\n')
            line_number++;
         p++;
      }
   } while (*p);

   return root;
}

/*------------------------------------------------------------------*/

/**
 * parse !ENTYTY entries of XML files and replace with references.
 * Return 0 in case of no errors, return error description.
 * Optional file_name is used for error reporting if called from mxml_parse_file()
 */
int mxml_parse_entity(char **buf, const char *file_name, char *error, int error_size, int *error_line)
{
   char *p;
   char *pv;
   char delimiter;
   int i, j, k, line_number, status;
   char *replacement;
   char entity_name[MXML_MAX_ENTITY][256];
   char entity_reference_name[MXML_MAX_ENTITY][256];
   char *entity_value[MXML_MAX_ENTITY];
   int entity_type[MXML_MAX_ENTITY];    /* internal or external */
   int entity_line_number[MXML_MAX_ENTITY];
   int nentity;
   int fh, length, len;
   char *buffer;
   int ip;                      /* counter for entity value */
   char directoryname[FILENAME_MAX];
   char filename[FILENAME_MAX];
   int entity_value_length[MXML_MAX_ENTITY];
   int entity_name_length[MXML_MAX_ENTITY];

   PMXML_NODE root = mxml_create_root_node();   /* dummy for 'HERE' */

   for (ip = 0; ip < MXML_MAX_ENTITY; ip++)
      entity_value[ip] = NULL;

   line_number = 1;
   nentity = -1;
   status = 0;

   if (!buf || !(*buf) || !strlen(*buf))
      return 0;

   strcpy(directoryname, file_name);
   mxml_dirname(directoryname);

   /* copy string to temporary space */
   buffer = (char *) mxml_malloc(strlen(*buf) + 1);
   if (buffer == NULL) {
      read_error(HERE, "Cannot allocate memory.");
      status = 1;
      goto error;
   }
   strcpy(buffer, *buf);

   p = strstr(buffer, "!DOCTYPE");
   if (p == NULL) {             /* no entities */
      status = 0;
      goto error;
   }

   pv = strstr(p, "[");
   if (pv == NULL) {            /* no entities */
      status = 1;
      goto error;
   }

   p = pv + 1;

   /* search !ENTITY */
   do {
      if (*p == ']')
         break;

      if (*p == '<') {

         /* found new entity */
         p++;
         while (*p && isspace((unsigned char)*p)) {
            if (*p == '\n')
               line_number++;
            p++;
         }
         if (!*p) {
            read_error(HERE, "Unexpected end of file");
            status = 1;
            goto error;
         }

         if (strncmp(p, "!--", 3) == 0) {
            /* found comment */
            p += 3;
            if (strstr(p, "-->") == NULL) {
               read_error(HERE, "Unterminated comment");
               status = 1;
               goto error;
            }

            while (strncmp(p, "-->", 3) != 0) {
               if (*p == '\n')
                  line_number++;
               p++;
            }
            p += 3;
         }

         else if (strncmp(p, "!ENTITY", 7) == 0) {
            /* found entity */
            nentity++;
            if (nentity >= MXML_MAX_ENTITY) {
               read_error(HERE, "Too much entities");
               status = 1;
               goto error;
            }
  
            entity_line_number[nentity] = line_number;
            
            pv = p + 7;
            while (*pv == ' ')
               pv++;

            /* extract entity name */
            p = pv;

            while (*p && isspace((unsigned char)*p) && *p != '<' && *p != '>') {
               if (*p == '\n')
                  line_number++;
               p++;
            }
            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*p == '<' || *p == '>') {
               read_error(HERE, "Unexpected \'%c\' inside !ENTITY", *p);
               status = 1;
               goto error;
            }

            pv = p;
            while (*pv && !isspace((unsigned char)*pv) && *pv != '<' && *pv != '>')
               pv++;

            if (!*pv) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*pv == '<' || *pv == '>') {
               read_error(HERE, "Unexpected \'%c\' inside entity \"%s\"", *pv, &entity_name[nentity][1]);
               status = 1;
               goto error;
            }

            entity_name[nentity][0] = '&';
            i = 1;
            entity_name[nentity][i] = 0;
            while (*p && !isspace((unsigned char)*p) && *p != '/' && *p != '>' && *p != '<' && i < 253)
               entity_name[nentity][i++] = *p++;
            entity_name[nentity][i++] = ';';
            entity_name[nentity][i] = 0;

            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*p == '<') {
               read_error(HERE, "Unexpected \'<\' inside entity \"%s\"", &entity_name[nentity][1]);
               status = 1;
               goto error;
            }

            /* extract replacement or SYSTEM */
            while (*p && isspace((unsigned char)*p)) {
               if (*p == '\n')
                  line_number++;
               p++;
            }
            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*p == '>') {
               read_error(HERE, "Unexpected \'>\' inside entity \"%s\"", &entity_name[nentity][1]);
               status = 1;
               goto error;
            }

            /* check if SYSTEM */
            if (strncmp(p, "SYSTEM", 6) == 0) {
               entity_type[nentity] = EXTERNAL_ENTITY;
               p += 6;
            } else {
               entity_type[nentity] = INTERNAL_ENTITY;
            }

            /* extract replacement */
            while (*p && isspace((unsigned char)*p)) {
               if (*p == '\n')
                  line_number++;
               p++;
            }
            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*p == '>') {
               read_error(HERE, "Unexpected \'>\' inside entity \"%s\"", &entity_name[nentity][1]);
               status = 1;
               goto error;
            }

            if (*p != '\"' && *p != '\'') {
               read_error(HERE, "Replacement was not found for entity \"%s\"", &entity_name[nentity][1]);
               status = 1;
               goto error;
            }
            delimiter = *p;
            p++;
            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            pv = p;
            while (*pv && *pv != delimiter)
               pv++;

            if (!*pv) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
            if (*pv == '<') {
               read_error(HERE, "Unexpected \'%c\' inside entity \"%s\"", *pv, &entity_name[nentity][1]);
               status = 1;
               goto error;
            }

            len = (int)((size_t) pv - (size_t) p);
            replacement = (char *) mxml_malloc(len + 1);
            if (replacement == NULL) {
               read_error(HERE, "Cannot allocate memory.");
               status = 1;
               goto error;
            }

            memcpy(replacement, p, len);
            replacement[len] = 0;
            mxml_decode(replacement);

            if (entity_type[nentity] == EXTERNAL_ENTITY) {
               strcpy(entity_reference_name[nentity], replacement);
            } else {
               entity_value[nentity] = (char *) mxml_malloc(strlen(replacement));
               if (entity_value[nentity] == NULL) {
                  read_error(HERE, "Cannot allocate memory.");
                  status = 1;
                  goto error;
               }
               strcpy(entity_value[nentity], replacement);
            }
            mxml_free(replacement);

            p = pv;
            while (*p && isspace((unsigned char)*p)) {
               if (*p == '\n')
                  line_number++;
               p++;
            }
            if (!*p) {
               read_error(HERE, "Unexpected end of file");
               status = 1;
               goto error;
            }
         }
      }

      /* go to next element */
      while (*p && *p != '<') {
         if (*p == '\n')
            line_number++;
         p++;
      }
   } while (*p);
   nentity++;

   /* read external file */
   for (i = 0; i < nentity; i++) {
      if (entity_type[i] == EXTERNAL_ENTITY) {
         if ( entity_reference_name[i][0] == DIR_SEPARATOR ) /* absolute path */
            strcpy(filename, entity_reference_name[i]);
         else /* relative path */
            sprintf(filename, "%s%c%s", directoryname, DIR_SEPARATOR, entity_reference_name[i]);
         fh = open(filename, O_RDONLY | O_TEXT, 0644);

         if (fh == -1) {
            line_number = entity_line_number[i];
            read_error(HERE, "%s is missing", entity_reference_name[i]);
            status = 1;
            goto error;
         } else {
            length = (int)lseek(fh, 0, SEEK_END);
            lseek(fh, 0, SEEK_SET);
            if (length == 0) {
               entity_value[i] = (char *) mxml_malloc(1);
               if (entity_value[i] == NULL) {
                  read_error(HERE, "Cannot allocate memory.");
                  close(fh);
                  status = 1;
                  goto error;
               }
               entity_value[i][0] = 0;
            } else {
               entity_value[i] = (char *) mxml_malloc(length);
               if (entity_value[i] == NULL) {
                  read_error(HERE, "Cannot allocate memory.");
                  close(fh);
                  status = 1;
                  goto error;
               }

               /* read complete file at once */
               length = (int)read(fh, entity_value[i], length);
               entity_value[i][length - 1] = 0;
               close(fh);

               /* recursive parse */
               if (mxml_parse_entity(&entity_value[i], filename, error, error_size, error_line) != 0) {
                  status = 1;
                  goto error;
               }
            }
         }
      }
   }

   /* count length of output string */
   length = (int)strlen(buffer);
   for (i = 0; i < nentity; i++) {
      p = buffer;
      entity_value_length[i] = (int)strlen(entity_value[i]);
      entity_name_length[i] = (int)strlen(entity_name[i]);
      while (1) {
         pv = strstr(p, entity_name[i]);
         if (pv) {
            length += entity_value_length[i] - entity_name_length[i];
            p = pv + 1;
         } else {
            break;
         }
      }
   }

   /* re-allocate memory */
   *buf = (char *) mxml_realloc(*buf, length + 1);
   if (*buf == NULL) {
      read_error(HERE, "Cannot allocate memory.");
      status = 1;
      goto error;
   }

   /* replace entities */
   p = buffer;
   pv = *buf;
   do {
      if (*p == '&') {
         /* found entity */
         for (j = 0; j < nentity; j++) {
            if (strncmp(p, entity_name[j], entity_name_length[j]) == 0) {
               for (k = 0; k < (int) entity_value_length[j]; k++)
                  *pv++ = entity_value[j][k];
               p += entity_name_length[j];
               break;
            }
         }
      }
      *pv++ = *p++;
   } while (*p);
   *pv = 0;

error:

   if (buffer != NULL)
      mxml_free(buffer);
   for (ip = 0; ip < MXML_MAX_ENTITY; ip++)
      if (entity_value[ip] != NULL)
         mxml_free(entity_value[ip]);

   return status;
}

/*------------------------------------------------------------------*/

/**
 * parse a XML file and convert it into a tree of MXML_NODE's.
 * Return NULL in case of an error, return error description
 */
PMXML_NODE mxml_parse_file(const char *file_name, char *error, int error_size, int *error_line)
{
   char *buf, line[1000];
   int fh, length;
   PMXML_NODE root;

   if (error)
      error[0] = 0;

   fh = open(file_name, O_RDONLY | O_TEXT, 0644);

   if (fh == -1) {
      sprintf(line, "Unable to open file \"%s\": ", file_name);
      strlcat(line, strerror(errno), sizeof(line));
      strlcpy(error, line, error_size);
      return NULL;
   }

   length = (int)lseek(fh, 0, SEEK_END);
   lseek(fh, 0, SEEK_SET);
   buf = (char *)mxml_malloc(length+1);
   if (buf == NULL) {
      close(fh);
      sprintf(line, "Cannot allocate buffer: ");
      strlcat(line, strerror(errno), sizeof(line));
      strlcpy(error, line, error_size);
      return NULL;
   }

   /* read complete file at once */
   length = (int)read(fh, buf, length);
   buf[length] = 0;
   close(fh);

   if (mxml_parse_entity(&buf, file_name, error, error_size, error_line) != 0) {
      mxml_free(buf);
      return NULL;
   }

   root = mxml_parse_buffer(buf, error, error_size, error_line);

   mxml_free(buf);

   return root;
}

/*------------------------------------------------------------------*/

/**
 * write complete subtree recursively into file opened with mxml_open_document()
 */
int mxml_write_subtree(MXML_WRITER *writer, PMXML_NODE tree, int indent)
{
   int i;

   mxml_start_element1(writer, tree->name, indent);
   for (i=0 ; i<tree->n_attributes ; i++)
      if (!mxml_write_attribute(writer, tree->attribute_name+i*MXML_NAME_LENGTH, tree->attribute_value[i]))
         return FALSE;
   
   if (tree->value)
      if (!mxml_write_value(writer, tree->value))
         return FALSE;

   for (i=0 ; i<tree->n_children ; i++)
      if (!mxml_write_subtree(writer, &tree->child[i], (tree->value == NULL) || i > 0))
         return FALSE;

   return mxml_end_element(writer);
}

/*------------------------------------------------------------------*/

/**
 * write a complete XML tree to a file
 */
int mxml_write_tree(const char *file_name, PMXML_NODE tree)
{
   MXML_WRITER *writer;
   int i;

   assert(tree);
   writer = mxml_open_file(file_name);
   if (!writer)
      return FALSE;

   for (i=0 ; i<tree->n_children ; i++)
     if (tree->child[i].node_type == ELEMENT_NODE) /* skip PI and comments */
         if (!mxml_write_subtree(writer, &tree->child[i], TRUE))
            return FALSE;

   if (!mxml_close_file(writer))
      return FALSE;

   return TRUE;
}

/*------------------------------------------------------------------*/

PMXML_NODE mxml_clone_tree(PMXML_NODE tree)
{
   PMXML_NODE clone;
   int i;

   clone = (PMXML_NODE)calloc(sizeof(MXML_NODE), 1);

   /* copy name, node_type, n_attributes and n_children */
   memcpy(clone, tree, sizeof(MXML_NODE));

   clone->value = NULL;
   mxml_replace_node_value(clone, tree->value);

   clone->attribute_name = NULL;
   clone->attribute_value = NULL;
   for (i=0 ; i<tree->n_attributes ; i++)
      mxml_add_attribute(clone, tree->attribute_name+i*MXML_NAME_LENGTH, tree->attribute_value[i]);

   clone->child = NULL;
   clone->n_children = 0;
   for (i=0 ; i<tree->n_children ; i++)
      mxml_add_tree(clone, mxml_clone_tree(mxml_subnode(tree, i)));

   return clone;
}

/*------------------------------------------------------------------*/

/**
 * print XML tree for debugging
 */
void mxml_debug_tree(PMXML_NODE tree, int level)
{
   int i, j;

   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Name: %s\n", tree->name);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Valu: %s\n", tree->value);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Type: %d\n", tree->node_type);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Lin1: %d\n", tree->line_number_start);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Lin2: %d\n", tree->line_number_end);

   for (j=0 ; j<tree->n_attributes ; j++) {
      for (i=0 ; i<level ; i++)
         printf("  ");
      printf("%s: %s\n", tree->attribute_name+j*MXML_NAME_LENGTH, 
         tree->attribute_value[j]);
   }

   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Addr: %08zX\n", (size_t)tree);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("Prnt: %08zX\n", (size_t)tree->parent);
   for (i=0 ; i<level ; i++)
      printf("  ");
   printf("NCld: %d\n", tree->n_children);

   for (i=0 ; i<tree->n_children ; i++)
      mxml_debug_tree(tree->child+i, level+1);

   if (level == 0)
      printf("\n");
}

/*------------------------------------------------------------------*/

/**
 * free memory of XML tree, must be called after any 
 * mxml_create_root_node() or mxml_parse_file()
 */
void mxml_free_tree(PMXML_NODE tree)
{
   int i;

   /* first free children recursively */
   for (i=0 ; i<tree->n_children ; i++)
      mxml_free_tree(&tree->child[i]);
   if (tree->n_children)
      mxml_free(tree->child);

   /* now free dynamic data */
   for (i=0 ; i<tree->n_attributes ; i++)
      mxml_free(tree->attribute_value[i]);

   if (tree->n_attributes) {
      mxml_free(tree->attribute_name);
      mxml_free(tree->attribute_value);
   }
   
   if (tree->value)
      mxml_free(tree->value);

   /* if we are the root node, free it */
   if (tree->parent == NULL)
      mxml_free(tree);
}

/*------------------------------------------------------------------*/

/*
void mxml_test()
{
   char err[256];
   PMXML_NODE tree, tree2, node;

   tree = mxml_parse_file("c:\\tmp\\test.xml", err, sizeof(err));
   tree2 = mxml_clone_tree(tree);

   printf("Orig:\n");
   mxml_debug_tree(tree, 0);

   printf("\nClone:\n");
   mxml_debug_tree(tree2, 0);

   printf("\nCombined:\n");
   node = mxml_find_node(tree2, "cddb"); 
   mxml_add_tree(tree, node);
   mxml_debug_tree(tree, 0);

   mxml_free_tree(tree);
}
*/

/*------------------------------------------------------------------*/
 /**
   mxml_basename deletes any prefix ending with the last slash '/' character
   present in path. mxml_dirname deletes the filename portion, beginning with
   the last slash '/' character to the end of path. Followings are examples
   from these functions

    path               dirname   basename
    "/"                "/"       ""
    "."                "."       "."
    ""                 ""        ""
    "/test.txt"        "/"       "test.txt"
    "path/to/test.txt" "path/to" "test.txt"
    "test.txt          "."       "test.txt"

   Under Windows, '\\' and ':' are recognized ad separator too.
 */

void mxml_basename(char *path)
{
   char str[FILENAME_MAX];
   char *p;
   char *name;

   if (path) {
      strcpy(str, path);
      p = str;
      name = str;
      while (1) {
         if (*p == 0)
            break;
         if (*p == '/'
#ifdef _MSC_VER
             || *p == ':' || *p == '\\'
#endif
             )
            name = p + 1;
         p++;
      }
      strcpy(path, name);
   }

   return;
}

void mxml_dirname(char *path)
{
   char *p;
#ifdef _MSC_VER
   char *pv;
#endif

   if (!path || strlen(path) == 0)
      return;

   p = strrchr(path, '/');
#ifdef _MSC_VER
   pv = strrchr(path, ':');
   if (pv > p)
      p = pv;
   pv = strrchr(path, '\\');
   if (pv > p)
      p = pv;
#endif

   if (p == 0)                  /* current directory */
      strcpy(path, ".");
   else if (p == path)          /* root directory */
      sprintf(path, "%c", *p);
   else
      *p = 0;

   return;
}

/*------------------------------------------------------------------*/

/**
 * Retieve node at a certain line number
 */
PMXML_NODE mxml_get_node_at_line(PMXML_NODE tree, int line_number)
{
   int i;
   PMXML_NODE pn;

   if (tree->line_number_start == line_number)
      return tree;
   
   for (i=0 ; i<tree->n_children ; i++) {
      pn = mxml_get_node_at_line(&tree->child[i], line_number);
      if (pn)
         return pn;
   }
    
   return NULL;
}

