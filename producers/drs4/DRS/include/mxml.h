/********************************************************************\

   Name:         mxml.h
   Created by:   Stefan Ritt
   Copyright 2000 + Stefan Ritt

   Contents:     Header file for mxml.c
   
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

/*------------------------------------------------------------------*/

#ifndef _MXML_H_
#define _MXML_H_

#define MXML_NAME_LENGTH 64

#define ELEMENT_NODE                  1
#define TEXT_NODE                     2
#define PROCESSING_INSTRUCTION_NODE   3
#define COMMENT_NODE                  4
#define DOCUMENT_NODE                 5

#define INTERNAL_ENTITY               0
#define EXTERNAL_ENTITY               1
#define MXML_MAX_ENTITY             500

#define MXML_MAX_CONDITION           10

#ifdef _MSC_VER
#define DIR_SEPARATOR              '\\'
#else
#define DIR_SEPARATOR               '/'
#endif

typedef struct {
   int  fh;
   char *buffer;
   int  buffer_size;
   int  buffer_len;
   int  level;
   int  element_is_open;
   int  data_was_written;
   char **stack;
   int  translate;
} MXML_WRITER;

typedef struct mxml_struct *PMXML_NODE;

typedef struct mxml_struct {
   char       name[MXML_NAME_LENGTH];  // name of element    <[name]>[value]</[name]>
   int        node_type;               // type of node XXX_NODE
   char       *value;                  // value of element
   int        n_attributes;            // list of attributes
   char       *attribute_name;
   char       **attribute_value;
   int        line_number_start;       // first line number in XML file, starting from 1
   int        line_number_end;         // last line number in XML file, starting from 1
   PMXML_NODE parent;                  // pointer to parent element
   int        n_children;              // list of children
   PMXML_NODE child;
} MXML_NODE;

/*------------------------------------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPRT
#if defined(EXPORT_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif
#endif

void mxml_suppress_date(int suppress);
MXML_WRITER *mxml_open_file(const char *file_name);
MXML_WRITER *mxml_open_buffer(void); 
int mxml_set_translate(MXML_WRITER *writer, int flag);
int mxml_start_element(MXML_WRITER *writer, const char *name);
int mxml_start_element_noindent(MXML_WRITER *writer, const char *name);
int mxml_end_element(MXML_WRITER *writer); 
int mxml_write_comment(MXML_WRITER *writer, const char *string);
int mxml_write_element(MXML_WRITER *writer, const char *name, const char *value);
int mxml_write_attribute(MXML_WRITER *writer, const char *name, const char *value);
int mxml_write_value(MXML_WRITER *writer, const char *value);
int mxml_write_empty_line(MXML_WRITER *writer);
char *mxml_close_buffer(MXML_WRITER *writer);
int mxml_close_file(MXML_WRITER *writer);

int mxml_get_number_of_children(PMXML_NODE pnode);
PMXML_NODE mxml_get_parent(PMXML_NODE pnode);
PMXML_NODE mxml_subnode(PMXML_NODE pnode, int idx);
PMXML_NODE mxml_find_node(PMXML_NODE tree, const char *xml_path);
int mxml_find_nodes(PMXML_NODE tree, const char *xml_path, PMXML_NODE **nodelist);
char *mxml_get_name(PMXML_NODE pnode);
char *mxml_get_value(PMXML_NODE pnode);
int mxml_get_line_number_start(PMXML_NODE pnode);
int mxml_get_line_number_end(PMXML_NODE pnode);
PMXML_NODE mxml_get_node_at_line(PMXML_NODE tree, int linenumber);
char *mxml_get_attribute(PMXML_NODE pnode, const char *name);

int mxml_add_attribute(PMXML_NODE pnode, const char *attrib_name, const char *attrib_value);
PMXML_NODE mxml_add_special_node(PMXML_NODE parent, int node_type, const char *node_name, const char *value);
PMXML_NODE mxml_add_special_node_at(PMXML_NODE parent, int node_type, const char *node_name, const char *value, int idx);
PMXML_NODE mxml_add_node(PMXML_NODE parent, const char *node_name, const char *value);
PMXML_NODE mxml_add_node_at(PMXML_NODE parent, const char *node_name, const char *value, int idx);

PMXML_NODE mxml_clone_tree(PMXML_NODE tree);
int mxml_add_tree(PMXML_NODE parent, PMXML_NODE tree);
int mxml_add_tree_at(PMXML_NODE parent, PMXML_NODE tree, int idx);

int mxml_replace_node_name(PMXML_NODE pnode, const char *new_name);
int mxml_replace_node_value(PMXML_NODE pnode, const char *value);
int mxml_replace_subvalue(PMXML_NODE pnode, const char *name, const char *value);
int mxml_replace_attribute_name(PMXML_NODE pnode, const char *old_name, const char *new_name);
int mxml_replace_attribute_value(PMXML_NODE pnode, const char *attrib_name, const char *attrib_value);

int mxml_delete_node(PMXML_NODE pnode);
int mxml_delete_attribute(PMXML_NODE, const char *attrib_name);

PMXML_NODE mxml_create_root_node(void);
PMXML_NODE mxml_parse_file(const char *file_name, char *error, int error_size, int *error_line);
PMXML_NODE mxml_parse_buffer(const char *buffer, char *error, int error_size, int *error_line);
int mxml_parse_entity(char **buf, const char* file_name, char *error, int error_size, int *error_line);
int mxml_write_tree(const char *file_name, PMXML_NODE tree);
void mxml_debug_tree(PMXML_NODE tree, int level);
void mxml_free_tree(PMXML_NODE tree);

void mxml_dirname(char* path);
void mxml_basename(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _MXML_H_ */
/*------------------------------------------------------------------*/
