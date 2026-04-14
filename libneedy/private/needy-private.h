//
// Created by vscode on 4/9/26.
//

#ifndef NEEDYRESOURCES_NEEDY_PRIVATE_H
#include <stdio.h>
#include <jansson.h>
#define NEEDYRESOURCES_NEEDY_PRIVATE_H
#define ENSURE_NOTNULL_FULL(ptr, retval) {if(ptr == NULL) return retval;}
#define ENSURE_NOTNULL(ptr) ENSURE_NOTNULL_FULL(ptr,)
#define ENSURE_NOTNULL_MSG_RETVAL(ptr, msg, ret) {if(ptr==NULL) { puts(msg); return ret; }}
#define ENSURE_NOTNULL_MSG_RNULL(ptr, msg) ENSURE_NOTNULL_MSG_RETVAL(ptr,msg,NULL)
#define ENSURE_NOTNULL_RNULL(ptr) ENSURE_NOTNULL_FULL(ptr, NULL)
#define ENSURE_NOTNULL_MSG(ptr,msg) ENSURE_NOTNULL_MSG_RETVAL(ptr,msg,;)


#endif //NEEDYRESOURCES_NEEDY_PRIVATE_H
