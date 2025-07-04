#include "typesdef.h"
#include "sys_config.h"
#include "list.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/sleep.h"
#include "osal/work.h"
#include "lib/common/sysevt.h"
#include "lib/net/uhttpd/uhttpd.h"

#if SYS_APP_UHTTPD

#include "html_bytes.c"

static const struct uhttpd_html uhttpd_htmls[] = {
   {.name = "index.html", .size = sizeof(index_bytes), .html = index_bytes},
};

const struct uhttpd_html *uhttpd_find_html(char *url)
{
    int32 i = 0;
    const struct uhttpd_html *html = NULL;

    if (ARRAY_SIZE(uhttpd_htmls) > 0) {
        html = &uhttpd_htmls[0];
        for (i = 0; i < ARRAY_SIZE(uhttpd_htmls); i++) {
            if (os_strcmp(uhttpd_htmls[i].name, "index.html") == 0) {
                html = &uhttpd_htmls[i];
            }
            if (os_strcmp(url, uhttpd_htmls[i].name) == 0) {
                html = &uhttpd_htmls[i];
                break;
            }
        }
    }

    return html;
}

#endif
