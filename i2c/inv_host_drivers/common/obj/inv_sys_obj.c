/*****************************************************************************
 *                                                                           *
 * INVECAS CONFIDENTIAL                                                      *
 * ____________________                                                      *
 *                                                                           *
 * [2018] - [2023] INVECAS, Incorporated                                     *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * NOTICE:  All information contained herein is, and remains                 *
 * the property of INVECAS, Incorporated, its licensors and suppliers,       *
 * if any.  The intellectual and technical concepts contained                *
 * herein are proprietary to INVECAS, Incorporated, its licensors            *
 * and suppliers and may be covered by U.S. and Foreign Patents,             *
 * patents in process, and are protected by trade secret or copyright law.   *
 * Dissemination of this information or reproduction of this material        *
 * is strictly forbidden unless prior written permission is obtained         *
 * from INVECAS, Incorporated.                                               *
 *                                                                           *
 *****************************************************************************/
/**
* file inv_sys_obj.c
*
* brief :- Object creater
*
*****************************************************************************/
/*#define INV_DEBUG 3*/

/***** #include statements ***************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "inv_common.h"
#include "inv_sys_obj_api.h"
#include "inv_sys_malloc_api.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (lib_obj, 10);

/***** local macro definitions ***********************************************/

#define LIST_OBJ2INST(p_list_obj)    ((inv_inst_t)INV_OBJ2INST(p_list_obj))
#define LIST_INST2OBJ(list_inst)    ((struct lst_obj *)INV_INST2OBJ(list_inst))

#define OBJECT2HEADER(p_void)       ((struct hdr_obj *)((char *)p_void-sizeof(struct hdr_obj)))
#define HEADER2OBJECT(p_hdr_obj)     ((void *)(((char *)p_hdr_obj)+sizeof(struct hdr_obj)))


/***** local type definitions ************************************************/

struct hdr_obj {
    const char *p_inst_str;
    inv_inst_t list_inst;
    inv_inst_t parent_inst;
    struct hdr_obj *p_hdr_obj_next;
};

struct lst_obj {
    const char *p_class_str;
    struct hdr_obj *p_hdr_obj_last;
    struct lst_obj *p_lst_obj_next;
    inv_sys_obj_size obj_size;
};

/***** local prototypes ******************************************************/

static void s_add_list(struct lst_obj *p_lst_obj_new);
static void s_remove_list(struct lst_obj *p_lst_obj_del);
static struct hdr_obj *s_prev_header_get(struct hdr_obj *p_hdr_obj);
static void s_post_insert_header(struct hdr_obj *p_hdr_obj_des, struct hdr_obj *p_hdr_obj_src);
static void s_insert_header(struct hdr_obj *p_hdr_obj);
static void s_remove_header(struct hdr_obj *p_hdr_obj);

/***** local data objects ****************************************************/

static struct lst_obj *sp_lst_obj_first;

/***** public functions ******************************************************/

void *inv_sys_obj_singleton_create(const char *p_class_str, inv_inst_t parent_inst, inv_sys_obj_size size)
{
    struct lst_obj *p_lst_obj = NULL;
    void *p_void = NULL;

    /*
     * Create new list
     */
    p_lst_obj = (struct lst_obj *) inv_sys_malloc_create(sizeof(struct lst_obj));
    if (p_lst_obj) {
        struct hdr_obj *p_hdr_obj = NULL;

        p_lst_obj->p_class_str = p_class_str;
        p_lst_obj->p_hdr_obj_last = NULL;
        p_lst_obj->p_lst_obj_next = NULL;
        p_lst_obj->obj_size = size;

        /*
         * Create link between each list of instantiations
         */
        s_add_list(p_lst_obj);

        /*
         * Request memory for instance header
         */
        p_hdr_obj = (struct hdr_obj *) inv_sys_malloc_create(sizeof(struct hdr_obj) + size);
        if (p_hdr_obj) {
            /*
             * Configure header
             */
            p_hdr_obj->p_inst_str = "Singleton";
            p_hdr_obj->list_inst = LIST_OBJ2INST (p_lst_obj);
            p_hdr_obj->parent_inst = parent_inst;
            p_hdr_obj->p_hdr_obj_next = NULL;

            /*
             * Insert instance to linked list
             */
            s_insert_header(p_hdr_obj);

            /*
             * Make sure that object is linked-in
             */
            INV_ASSERT (p_hdr_obj->p_hdr_obj_next);

            p_void = HEADER2OBJECT (p_hdr_obj);
        }
    }
    return p_void;
}

void inv_sys_obj_singleton_delete(void *p_obj)
{
    struct hdr_obj *p_hdr_obj;
    struct lst_obj *p_lst_obj;

    INV_ASSERT (p_obj);
    p_hdr_obj = OBJECT2HEADER (p_obj);
    p_lst_obj = LIST_INST2OBJ (p_hdr_obj->list_inst);

    /*
     * Remove instance from linked list
     */
    INV_ASSERT (p_hdr_obj->p_hdr_obj_next);
    s_remove_header(p_hdr_obj);
    INV_ASSERT (!p_hdr_obj->p_hdr_obj_next);

    /*
     * Delete instance memory
     */
    inv_sys_malloc_delete(p_hdr_obj);

    /*
     * Remove list
     */
    s_remove_list(p_lst_obj);

    /*
     * Delete list memory
     */
    inv_sys_malloc_delete(p_lst_obj);
}

inv_inst_t inv_sys_obj_list_create(const char *p_class_str, inv_sys_obj_size size)
{
    struct lst_obj *p_lst_obj;

    /*
     * Create new list
     */
    p_lst_obj = (struct lst_obj *) inv_sys_malloc_create(sizeof(struct lst_obj));
    INV_ASSERT (p_lst_obj);

    if (p_lst_obj) {
        p_lst_obj->p_class_str = p_class_str;
        p_lst_obj->p_hdr_obj_last = NULL;
        p_lst_obj->p_lst_obj_next = NULL;
        p_lst_obj->obj_size = size;

        /*
         * Create link between each list of instantiations
         */
        s_add_list(p_lst_obj);
    }
    return LIST_OBJ2INST (p_lst_obj);
}

void inv_sys_obj_list_delete(inv_inst_t list_inst)
{
    struct lst_obj *p_lst_obj = LIST_INST2OBJ (list_inst);

    INV_ASSERT (p_lst_obj);

    /*
     * Remove all existing linked instances
     */
    while (p_lst_obj->p_hdr_obj_last) {
        /*
         * Remove instance from linked list
         */
        s_remove_header(p_lst_obj->p_hdr_obj_last);

        /*
         * Delete instance memory
         */
        inv_sys_malloc_delete(p_lst_obj->p_hdr_obj_last);
    }

    /*
     * Remove list
     */
    s_remove_list(p_lst_obj);

    /*
     * Delete list memory
     */
    inv_sys_malloc_delete(p_lst_obj);
}

void *inv_sys_obj_instance_create(inv_inst_t list_inst, inv_inst_t parent_inst, const char *p_inst_str)
{
    struct lst_obj *p_lst_obj = NULL;
    struct hdr_obj *p_hdr_obj = NULL;
    void *p_void = NULL;

    INV_ASSERT (list_inst);
    p_lst_obj = LIST_INST2OBJ (list_inst);

    /*
     * Request memory for instance header
     */
    p_hdr_obj = (struct hdr_obj *) inv_sys_malloc_create(sizeof(struct hdr_obj) + p_lst_obj->obj_size);
    if (p_hdr_obj) {
        /*
         * Configure header
         */
        p_hdr_obj->p_inst_str = p_inst_str;
        p_hdr_obj->list_inst = list_inst;
        p_hdr_obj->parent_inst = parent_inst;
        p_hdr_obj->p_hdr_obj_next = NULL;

        /*
         * Insert instance to linked list
         */
        s_insert_header(p_hdr_obj);

        /*
         * Make sure that object is linked-in
         */
        INV_ASSERT (p_hdr_obj->p_hdr_obj_next);

        p_void = HEADER2OBJECT (p_hdr_obj);
    }
    return p_void;
}

void inv_sys_obj_instance_delete(void *p_obj)
{
    struct hdr_obj *p_hdr_obj;

    INV_ASSERT (p_obj);
    p_hdr_obj = OBJECT2HEADER (p_obj);

    /*
     * Remove instance from linked list
     */
    INV_ASSERT (p_hdr_obj->p_hdr_obj_next);
    s_remove_header(p_hdr_obj);
    INV_ASSERT (!p_hdr_obj->p_hdr_obj_next);

    /*
     * Delete instance memory
     */
    inv_sys_malloc_delete(p_hdr_obj);
}

void *inv_sys_obj_first_get(inv_inst_t list_inst)
{
    struct lst_obj *p_lst_obj = LIST_INST2OBJ (list_inst);

    return (p_lst_obj->p_hdr_obj_last) ? (HEADER2OBJECT (p_lst_obj->p_hdr_obj_last->p_hdr_obj_next)) : (NULL);
}

#if (INV_ENV_BUILD_ASSERT)
bool_t inv_sys_obj_check(inv_inst_t list_inst, void *p_obj)
{
    struct lst_obj *p_lst_obj = NULL;

    INV_ASSERT (list_inst);
    p_lst_obj = LIST_INST2OBJ (list_inst);
    if (p_lst_obj) {
        struct hdr_obj *p_hdr_obj = OBJECT2HEADER (p_obj);

        /*
         * Check if object is registered to the same list
         */
        if (list_inst == p_hdr_obj->list_inst)
            return true;
    }
    return false;
}
#endif

inv_inst_t inv_sys_obj_parent_inst_get(void *p_obj)
{
    return OBJECT2HEADER (p_obj)->parent_inst;
}




const char *inv_sys_obj_name_get(void *p_obj)
{
    struct hdr_obj *p_hdr_obj = NULL;

    INV_ASSERT (p_obj);
    p_hdr_obj = OBJECT2HEADER (p_obj);
    return p_hdr_obj->p_inst_str;
}

void *inv_sys_obj_next_get(void *p_obj)
{
    struct hdr_obj *p_hdr_obj = NULL;
    struct lst_obj *p_lst_obj = NULL;

    INV_ASSERT (p_obj);
    p_hdr_obj = OBJECT2HEADER (p_obj);
    p_lst_obj = LIST_INST2OBJ (p_hdr_obj->list_inst);
    INV_ASSERT (p_lst_obj);
    if (p_hdr_obj != p_lst_obj->p_hdr_obj_last)
        return HEADER2OBJECT (p_hdr_obj->p_hdr_obj_next);

    /*
     * Reached end of list
     */
    return NULL;
}

void inv_sys_obj_move(void *p_obj_des, void *p_obj_src)
{
    struct hdr_obj *p_hdr_obj_src = NULL;
    struct lst_obj *p_lst_obj = NULL;

    INV_ASSERT (p_obj_src);

    /*
     * No mvove is needed if p_src and p_des pointing to the same object
     */
    if (p_obj_des == p_obj_src)
        return;

    p_hdr_obj_src = OBJECT2HEADER (p_obj_src);
    p_lst_obj = LIST_INST2OBJ (p_hdr_obj_src->list_inst);

    /*
     * Make sure that source object is currently linked in
     */
    INV_ASSERT (p_hdr_obj_src->p_hdr_obj_next);

    /*
     * Temporarily remove source object from linked list
     */
    s_remove_header(p_hdr_obj_src);

    /*
     * With a valid destination object provided the source object will be replaced to directly behind the destination object.
     */
    /*
     * However if null pointer is provided for destination object, then source object will be moved to the first position of list.
     */
    if (p_obj_des) {
        struct hdr_obj *p_hdr_obj_des = OBJECT2HEADER (p_obj_des);

        /*
         * Make sure that both instantiations belong to the same list(type)
         */
        INV_ASSERT (p_hdr_obj_des->list_inst == p_hdr_obj_src->list_inst);

        /*
         * Make sure that destination object is a linked in object
         */
        INV_ASSERT (p_hdr_obj_des->p_hdr_obj_next);

        /*
         * Insert in linked list
         */
        s_post_insert_header(p_hdr_obj_des, p_hdr_obj_src);

        /*
         * Update last pointer if inserted at end of list
         */
        if (p_hdr_obj_des == p_lst_obj->p_hdr_obj_last)
            p_lst_obj->p_hdr_obj_last = p_hdr_obj_src;
    } else {
        if (p_lst_obj->p_hdr_obj_last)
            /*
             * Insert as first object in list
             */
            s_post_insert_header(p_lst_obj->p_hdr_obj_last, p_hdr_obj_src);
        else {
            /*
             * First inserted object should point to itself to ensure a circular linked list
             */
            p_hdr_obj_src->p_hdr_obj_next = p_hdr_obj_src;
            p_lst_obj->p_hdr_obj_last = p_hdr_obj_src;
        }
    }
}

/***** local functions *******************************************************/

static void s_add_list(struct lst_obj *p_lst_obj_new)
{
    if (sp_lst_obj_first) {
        struct lst_obj *p_lst_obj = sp_lst_obj_first;

        while ((p_lst_obj->p_lst_obj_next))
            p_lst_obj = p_lst_obj->p_lst_obj_next;
        p_lst_obj->p_lst_obj_next = p_lst_obj_new;
    } else
        sp_lst_obj_first = p_lst_obj_new;
}

static void s_remove_list(struct lst_obj *p_lst_obj_del)
{
    /*
     * Make sure that at least one list exists
     */
    INV_ASSERT (sp_lst_obj_first);
    if (sp_lst_obj_first == p_lst_obj_del)
        sp_lst_obj_first = sp_lst_obj_first->p_lst_obj_next;
    else {
        struct lst_obj *p_lst_obj = sp_lst_obj_first;

        /*
         * search linked list
         */
        while (p_lst_obj->p_lst_obj_next) {
            if (p_lst_obj->p_lst_obj_next == p_lst_obj_del)
                break;

            p_lst_obj = p_lst_obj->p_lst_obj_next;

        }
        /*
         * Generate assertion if provided obj-list is not found
         */
        INV_ASSERT (p_lst_obj->p_lst_obj_next);

        /*
         * remove and relink linked list
         */
        p_lst_obj->p_lst_obj_next = p_lst_obj_del->p_lst_obj_next;
    }
}

static struct hdr_obj *s_prev_header_get(struct hdr_obj *p_hdr_obj)
{
    struct hdr_obj *p_hdr_obj_tmp = p_hdr_obj;

    while (p_hdr_obj != p_hdr_obj_tmp->p_hdr_obj_next)
        p_hdr_obj_tmp = p_hdr_obj_tmp->p_hdr_obj_next;

    return p_hdr_obj_tmp;
}

static void s_post_insert_header(struct hdr_obj *p_hdr_obj_des, struct hdr_obj *p_hdr_obj_src)
{
    p_hdr_obj_src->p_hdr_obj_next = p_hdr_obj_des->p_hdr_obj_next;
    p_hdr_obj_des->p_hdr_obj_next = p_hdr_obj_src;
}

static void s_insert_header(struct hdr_obj *p_hdr_obj)
{
    struct lst_obj *p_lst_obj = NULL;

    INV_ASSERT (p_hdr_obj);

    /*
     * Make sure that new header is un-linked
     */
    INV_ASSERT (!p_hdr_obj->p_hdr_obj_next);

    p_lst_obj = LIST_INST2OBJ (p_hdr_obj->list_inst);
    if (p_lst_obj->p_hdr_obj_last)
        /*
         * Insert at the end of list of headers
         */
        s_post_insert_header(p_lst_obj->p_hdr_obj_last, p_hdr_obj);
    else
        /*
         * First inserted object should point to itself to ensure a circular linked list
         */
        p_hdr_obj->p_hdr_obj_next = p_hdr_obj;
    p_lst_obj->p_hdr_obj_last = p_hdr_obj;
}

static void s_remove_header(struct hdr_obj *p_hdr_obj)
{
    struct lst_obj *p_lst_obj = NULL;

    INV_ASSERT (p_hdr_obj);

    /*
     * Make sure that header is currently linked in
     */
    INV_ASSERT (p_hdr_obj->p_hdr_obj_next);

    p_lst_obj = LIST_INST2OBJ (p_hdr_obj->list_inst);

    /*
     * check if this is the last object to be removed
     */
    if (p_hdr_obj == p_hdr_obj->p_hdr_obj_next)
        /*
         * Clear reference to linked list
         */
        p_lst_obj->p_hdr_obj_last = NULL;
    else {
        struct hdr_obj *p_hdr_obj_prv = s_prev_header_get(p_hdr_obj);

        /*
         * remove instance out of linked list
         */
        p_hdr_obj_prv->p_hdr_obj_next = p_hdr_obj->p_hdr_obj_next;

        /*
         * Change last instance reference in case last instance is removed
         */
        if (p_lst_obj->p_hdr_obj_last == p_hdr_obj)
            p_lst_obj->p_hdr_obj_last = p_hdr_obj_prv;
    }

    /*
     * Clear next reference to indicate that instance has been removed from list
     */
    p_hdr_obj->p_hdr_obj_next = NULL;
}

/***** end of file ***********************************************************/
