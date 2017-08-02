#include <assert.h>
#include <errno.h>

#include <libxml/parser.h>

#include <zip.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <glib.h>
#include <gio/gio.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-extension/nautilus-info-provider.h>

#include "nautilus-extension-epub.h"

typedef struct _EpubExtension EpubExtension;
typedef struct _EpubExtensionClass EpubExtensionClass;

typedef struct {
    GClosure *update_complete;
    NautilusInfoProvider *provider;
    NautilusFileInfo *file;
    int operation_handle;
    gboolean cancelled;
} UpdateHandle;

struct _EpubExtension
{
    GObject parent_slot;
};

struct _EpubExtensionClass
{
    GObjectClass parent_slot;
};

static GType provider_types[1];
static GType epub_extension_type;

/* Extension initialization */
void
nautilus_module_initialize (GTypeModule  *module)
{
    epub_extension_register_type(module);
    provider_types[0] = epub_extension_get_type();
    xmlInitParser();
    LIBXML_TEST_VERSION
}

void
nautilus_module_shutdown(void)
{
    /* Any module-specific shutdown */
    xmlCleanupParser();
}

void
nautilus_module_list_types(const GType **types, int *num_types)
{
    *types = provider_types;
    *num_types = G_N_ELEMENTS(provider_types);
}

/* Interfaces */
static void
epub_extension_column_provider_iface_init (NautilusColumnProviderIface *iface) {
    iface->get_columns = epub_extension_get_columns;
    return;
}

static void
epub_extension_info_provider_iface_init (NautilusInfoProviderIface *iface) {
    iface->update_file_info = epub_extension_update_file_info;
    iface->cancel_update = epub_extension_cancel_update;
    return;
}

#ifdef PROPERTY
static void
epub_extension_propertypage_provider_iface_init (NautilusPropertyPageProviderIface *iface)
{
    iface->get_pages = epub_extension_get_pages;
}
#endif

/* Extension */
static void
epub_extension_class_init(EpubExtensionClass *class)
{
}

static void
epub_extension_instance_init(EpubExtension *img)
{
}

static void
epub_extension_register_type(GTypeModule *module)
{
        static const GTypeInfo info = {
                                        sizeof(EpubExtensionClass),
                                        (GBaseInitFunc) NULL,
                                        (GBaseFinalizeFunc) NULL,
                                        (GClassInitFunc) epub_extension_class_init,
                                        NULL,
                                        NULL,
                                        sizeof (EpubExtension),
                                        0,
                                        (GInstanceInitFunc) epub_extension_instance_init,
                                        };

        static const GInterfaceInfo column_provider_iface_info = {
            (GInterfaceInitFunc) epub_extension_column_provider_iface_init,
            NULL,
            NULL
        };

        static const GInterfaceInfo info_provider_iface_info = {
            (GInterfaceInitFunc) epub_extension_info_provider_iface_init,
            NULL,
            NULL
        };

        epub_extension_type = g_type_module_register_type(module,
                                                        G_TYPE_OBJECT,
                                                        "EpubExtension",
                                                        &info, 0);

        /* ... add interfaces ... */
        g_type_module_add_interface (module,
                                    epub_extension_type,
                                    NAUTILUS_TYPE_COLUMN_PROVIDER,
                                    &column_provider_iface_info);

        g_type_module_add_interface (module,
                                     epub_extension_type,
                                     NAUTILUS_TYPE_INFO_PROVIDER,
                                     &info_provider_iface_info);
        #ifdef PROPERTY
        static const GInterfaceInfo property_provider_iface_info = {
            (GInterfaceInitFunc) epub_extension_propertypage_provider_iface_init,
            NULL,
            NULL
        };
        g_type_module_add_interface (module,
                                     epub_extension_type,
                                     NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
                                     &property_provider_iface_info);
        #endif
}

GType
epub_extension_get_type(void)
{
    return epub_extension_type;
}

/* Column interfaces */
static GList *
epub_extension_get_columns(NautilusColumnProvider *provider)
{
    NautilusColumn *column;
    GList *ret = NULL;
    column = nautilus_column_new("EpubExtension::epub_title_column",
                                 "EpubExtension::epub_title",
                                 "Epub title",
                                 "Epub title");
    ret = g_list_append(ret, column);
    column = nautilus_column_new("EpubExtension::epub_lang_column",
                                 "EpubExtension::epub_lang",
                                 "Epub lang",
                                 "Epub lang");
    column = nautilus_column_new("EpubExtension::epub_creator_column",
                                 "EpubExtension::epub_creator",
                                 "Epub lang",
                                 "Epub lang");
    ret = g_list_append(ret, column);
    return ret;
}

/* Info interfaces */
static void
epub_extension_cancel_update(NautilusInfoProvider *provider,
                             NautilusOperationHandle *handle)
{
    UpdateHandle *update_handle = (UpdateHandle*)handle;
    update_handle->cancelled = TRUE;
}

static NautilusOperationResult
epub_extension_update_file_info(NautilusInfoProvider *provider,
                NautilusFileInfo *file,
                GClosure *update_complete,
                NautilusOperationHandle **handle)
{
    if(nautilus_file_info_is_directory(file))
        return NAUTILUS_OPERATION_COMPLETE;
    if(!nautilus_file_info_is_mime_type(file, "application/epub+zip"))
        return NAUTILUS_OPERATION_COMPLETE;
    //char *title = nautilus_file_info_get_string_attribute(file, "EpubExtension::epub_title");
    char *title = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_title");
    char *lang = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_lang");
    char *creator = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_creator");

    /* get and provide the information associated with the column.
       If the operation is not fast enough, we should use the arguments 
       update_complete and handle for asyncrhnous operation. */
    if (!title) {
        UpdateHandle *update_handle = g_new0(UpdateHandle, 1);
        update_handle->update_complete = g_closure_ref(update_complete);
        update_handle->provider = provider;
        update_handle->file = g_object_ref(file);
        //g_timeout_add (1, 
        g_idle_add(
                timeout_epub_callback, 
                update_handle);
        *handle = update_handle;
        return NAUTILUS_OPERATION_IN_PROGRESS;
    }
    nautilus_file_info_add_string_attribute(file,
                                            "EpubExtension::epub_title",
                                            title);
    nautilus_file_info_add_string_attribute(handle->file,
                                            "EpubExtension::epub_lang",
                                            lang);
    nautilus_file_info_add_string_attribute(handle->file,
                                            "EpubExtension::epub_creator",
                                            creator);
    return NAUTILUS_OPERATION_COMPLETE;
}


/* Callback for async */
gint
timeout_epub_callback(gpointer data)
{
    UpdateHandle *handle = (UpdateHandle*)data;
    if (!handle->cancelled) {
        char *filename = g_file_get_path(nautilus_file_info_get_location(handle->file));
        EpubInfo info;
        const int result = read_from_epub(filename, &info);
        if(result == 0) {
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_title",
                                                     info.title);
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_lang",
                                                     info.lang);
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_creator",
                                                     info.creator);
        
            /* Cache the data so that we don't have to read it again */
            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_title",
                                            info.title);

            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_lang",
                                            info.lang);
            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_creator",
                                            info.creator);
        } else {
            if(result == 1) {
                nautilus_file_info_add_string_attribute (handle->file,
                                                        "EpubExtension::epub_title",
                                                         info.title);
                char *data_s = g_strdup_printf("%s", info.title);
                g_object_set_data(G_OBJECT(handle->file), 
                                        "EpubExtension::epub_title",
                                        data_s);
                g_free(data_s)
            } else {
                char *data_s = g_strdup_printf("%s, Code: %d", epub_errors[result], result);
                nautilus_file_info_add_string_attribute (handle->file,
                                                        "EpubExtension::epub_title",
                                                         data_s);
                g_object_set_data(G_OBJECT(handle->file), 
                                        "EpubExtension::epub_title",
                                        data_s);
                g_free(data_s)
            }
        }
        g_free(filename);
    }
    
    nautilus_info_provider_update_complete_invoke
                                                (handle->update_complete,
                                                 handle->provider,
                                                 (NautilusOperationHandle*)handle,
                                                 NAUTILUS_OPERATION_COMPLETE);
    /* We're done with the handle */
    g_closure_unref(handle->update_complete);
    g_object_unref(handle->file);
    g_free(handle);
    return 0;
}
/* Epub */

static int
read_from_epub(const char *archive, EpubInfo *info)
{
    int result = 0; /* Result for operation */
    /* Zip error */
    int err = 0;
    char errbuf[MAX_STR_LEN];
    char *dataEntry;
    /* Zip */
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    /* For files in zip */
    zip_int64_t num64; /* Number of files */
    zip_uint64_t i;    /* Counter, for 0..num64 */
    zip_uint64_t uncomp_size; /* Zize of uncompressed file */
    zip_int64_t fread_len; /* Really read bytes from zipped file. */
    int len;
    za = zip_open(archive, 0, &err);
    if (za == NULL) {
        zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
        strncpy(info->title, errbuf, MAX_STR_LEN);
        return 1;
    }
    /*Read container.xml*/
    zf = zip_fopen(za, "META-INF/container.xml", ZIP_FL_COMPRESSED);
    if(!zf) {
        zip_close(za);
        return 2;
    }
    AboutContainer container;
    container.my_state == INIT;
    char buffer[ZIP_BUFFER_LEN];
    memset(&container, 0, sizeof(AboutContainer));
    memset(buffer, 0, sizeof(buffer));
    xmlSAXHandler SAXHander;
    make_sax_handler_container(&SAXHander, &container);
 
    fread_len = zip_fread(zf, buffer, sizeof(buffer));
    if(fread_len <= 0) {
        zip_fclose(zf);
        zip_close(za);
        return(3);
    }
    xmlParserCtxtPtr ctxt = xmlCreatePushParserCtxt(
        &SAXHander, NULL, buffer, fread_len, NULL
    );    
    while(container.my_state != STOP) {
        fread_len = zip_fread(zf, buffer, sizeof(buffer));
        if(fread_len <= 0)
            break;
        if(xmlParseChunk(ctxt, buffer, fread_len, 0)) {
            xmlParserError(ctxt, "xmlParseChunk");
            zip_fclose(zf);
            zip_close(za);
            return 3;
        }
    }
    zip_fclose(zf);
    /* Read book info */
    zf = zip_fopen(za, container.contentFilename, ZIP_FL_COMPRESSED);
    if(!zf) {
        zip_close(za);
        return(3);
    }
    fread_len = zip_fread(zf, buffer, sizeof(buffer));
    if(fread_len <= 0) {
        zip_fclose(zf);
        zip_close(za);
        return(3);
    }
    make_sax_handler_contentOPF(&SAXHander, info);
    info.my_state = INIT;
    xmlParserCtxtPtr ctxt2 = xmlCreatePushParserCtxt(
        &SAXHander, NULL, buffer, fread_len, NULL
    );    
    while(info.my_state != STOP) {
        fread_len = zip_fread(zf, buffer, sizeof(buffer));
        if(fread_len <= 0)
            break;
        if(xmlParseChunk(ctxt2, buffer, fread_len, 0)) {
            xmlParserError(ctxt2, "xmlParseChunk");
            zip_fclose(zf);
            zip_close(za);
            return 4;
        }
    }
    zip_fclose(zf);
    /* End */
    if (zip_close(za) == -1) {
        return 5;
    }
    return result;
}

static void
make_sax_handler_container(xmlSAXHandler *SAXHander,
                           void *user_data)
{
    memset(SAXHander, 0, sizeof(xmlSAXHandler));
    SAXHander->initialized = XML_SAX2_MAGIC;
    SAXHander->startElementNs = OnStartElementContainerNs;
    SAXHander->_private = user_data;
}

static void
make_sax_handler_contentOPF(xmlSAXHandler *SAXHander,
                            void *user_data)
{
    memset(SAXHander, 0, sizeof(xmlSAXHandler));
    SAXHander->initialized = XML_SAX2_MAGIC;
    SAXHander->startElementNs = OnStartElementNs;
    SAXHander->endElementNs = OnEndElementNs;
    SAXHander->characters = OnCharacters;
    SAXHander->_private = user_data;
}

static void
my_strlcpy(char *dest, const char *src_begin, const char *src_end, size_t count)
{
    size_t i = 0;
    //for(char *a=(char *)src_begin; a < src_end && i < count; ++a, ++i)
    //dest[i] = *a;
    for(char *a = (char *)src_begin; a < src_end && i < count; ++a, ++i, ++dest)
        *dest = *a;
}

static int
my_strncmp(const char *lhs, const char *rhs_begin, const char *rhs_end, size_t count)
{
    /*See: http://cmcmsu.no-ip.info/2course/strcmp.feature.htm */
    char *l = (char *)lhs;
    char *r = (char *)rhs_begin;
    for(size_t i = 0; r! = rhs_end && i < count; ++i, ++l, ++r) {
        if((unsigned char)*l != (unsigned char)*r)
            return -1;
    }
    return 0;
}

static void
OnStartElementContainerNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
    )
{
    #ifdef DEBUG
    fprintf (stderr, "Event: OnStartElementContainerNs!\n");
    #endif
    char media_type[MAX_STR_LEN];
    memset(media_type, 0, sizeof(media_type));
    xmlSAXHandlerPtr handler = ((xmlParserCtxtPtr)ctx)->sax;
    AboutContainer *container = (AboutContainer *)(handler->_private);
    char *fname = (char *)(handler->_private);
    if(g_strcmp0((const char*)localname, "rootfile") == 0) {
        size_t index = 0;
        for(int indexAttr = 0;
            indexAttr < nb_attributes;
            ++indexAttr, index += 5) {
            const xmlChar *a_localname = attributes[index];
            const xmlChar *a_prefix = attributes[index+1];
            const xmlChar *a_nsURI = attributes[index+2];
            const xmlChar *a_valueBegin = attributes[index+3];
            const xmlChar *a_valueEnd = attributes[index+4];
            if(g_strcmp0((const char*)a_localname, "full-path") == 0) {
                my_strlcpy(AboutContainer->contentFilename, (const char *)a_valueBegin, (const char *)a_valueEnd, MAX_STR_LEN);
            }
            /*
            if(g_strcmp0((const char*)a_localname, "media-type") == 0) {
                my_strlcpy(media_type, (const char *)valueBegin, (const char *)valueEnd, MAX_STR_LEN);
                media_type_flag = 1;
            }
            */
            if( g_strcmp0((const char*)a_localname, "media-type") == 0 &&
                my_strncmp("application/oebps-package+xml", (const char *)a_valueBegin, (const char *)a_valueEnd, MAX_STR_LEN) == 0
                ) {
                container->my_state = STOP;
            }
        }
    }
    /*
    if(g_strcmp0((const char*)media_type, "application/oebps-package+xml") == 0) {
        xmlStopParser(ctx);
    }
    */
}

static inline int 
max(int a, int b)
{
    if(a > b)
        return a;
    return b;
}

static inline int 
min(int a, int b)
{
    if(a < b)
        return a;
    return b;
}

static void
my_strlcat_names(char *dest, size_t count, const char *src, size_t count2)
{
    size_t id = 0;
    char *d = dest;
    while(d != '\0') {
        ++d;
        ++id;
    }
    if(d != dest) {
        if(id!=count) {
            *d = ',';
            ++d;
            ++id;
        }
        if(id!=count) {
            *d = ' ';
            ++d;
            ++id;
        }
    }
    char *s = (char *)src;
    for(size_t is = 0; is < count2 && id < count && *src!='\0'; ++d, ++id, ++is, ++s)
        *d = *s;
    dest[count-1] = '\0';
}

static void
OnCharacters(void * ctx,
    const xmlChar * ch,
    int len)
{
    #ifdef DEBUG
    fprintf(stderr, "Event: OnCharacters!\n");
    #endif
    xmlSAXHandlerPtr handler = ((xmlParserCtxtPtr)ctx)->sax;
    EpubInfo *info = (EpubInfo *)(handler->_private);
    switch (info->my_state) {
    case CREATOR_OPENED: {
        const int len2 = min(len, MAX_STR_LEN);
        my_strlcat_names(info->creator, MAX_STR_LEN, (const char *)ch, len2);
        /* https://developer.gnome.org/glib/stable/glib-String-Utility-Functions.html#g-strlcat */
        //g_strlcat(info->creator, (const char *)ch, len2);
        /* Since C11 */
        /* strncat_s(info->creator, MAX_STR_LEN, (const char *)ch, MAX_STR_LEN); */
        info->my_state = CREATOR_END;
        }
        break;
    case BOOK_TITLE_OPENED: {
        const int len2 = min(len, MAX_STR_LEN);
        strncpy(info->title, (const char *)ch, len2);
        info->my_state = BOOK_TITLE_END;
        }
        break;
    case LANG_OPENED: {
        const int len2 = min(len, LANG_LENGTH);
        strncpy(info->lang, (const char *)ch, len2);
        info->my_state = LANG_END;
        }
        break;
    default:
        break;
    }
}

static void
OnStartElementNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
    )
{
    #ifdef DEBUG
    fprintf (stderr, "Event: OnStartElementNs!\n");
    #endif
    xmlSAXHandlerPtr handler = ((xmlParserCtxtPtr)ctx)->sax;
    EpubInfo *info = (EpubInfo *)(handler->_private);
    info->my_state = INIT;
    if (g_strcmp0((const char*)localname, "creator") == 0) {
        info->my_state = CREATOR_OPENED;
        return;
    }
    if (g_strcmp0((const char*)localname, "language") == 0) {
        info->my_state = LANG_OPENED;
        return;
    }
    if (g_strcmp0((const char*)localname, "title") == 0) {
        info->my_state = BOOK_TITLE_OPENED;
        return;
    }
}

static void
OnEndElementNs(
    void* ctx,
    const xmlChar* localname,
    const xmlChar* prefix,
    const xmlChar* URI
    )
{
    #ifdef DEBUG
    fprintf (stderr, "Event: OnEndElementNs!\n");
    #endif
    xmlSAXHandlerPtr handler = ((xmlParserCtxtPtr)ctx)->sax;
    EpubInfo *info = (EpubInfo *)(handler->_private);
    if (g_strcmp0((const char*)localname, "metadata") == 0) {
        info->my_state = STOP;
        xmlStopParser(ctx);
    }
}

#ifdef PROPERTY
static GList *
epub_extension_get_pages (NautilusPropertyPageProvider *provider,
                         GList *files)
{
    GList *pages;
    NautilusPropertyPage *page;
    NautilusFileInfo *file;
    GtkWidget *table;

    // only show property page when a single file is selected:
    if (!files || files->next != NULL) {
            return NULL;
    }
    
    file = files->data;

    /* only show property page for certain file types: */
    if (!nautilus_file_info_is_mime_type(file, "application/epub+zip")) {
            return NULL;
    }
    
    pages = NULL;
    /*gchar *name;
    name = nautilus_file_info_get_uri (file);

    properties = gtk_button_new_with_label (name);
    */
    gtk_widget_show(table);
    page = nautilus_property_page_new ("EpubExtension::property_page",
                                    gtk_label_new ("Epub info"),
                                    table);
    pages = g_list_append (pages, page);
    char *title = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_title");
    char *lang = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_lang");
    char *creator = g_object_get_data(G_OBJECT(file), "EpubExtension::epub_creator");
    if(!title) {
        char *filename = g_file_get_path(nautilus_file_info_get_location(handle->file));
        EpubInfo info;
        const int result = read_from_epub(filename, &info);
        if(result == 0) {
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_title",
                                                     info.title);
            title = info.title;
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_lang",
                                                     info.lang);
            lang = info.lang;
            nautilus_file_info_add_string_attribute(handle->file,
                                                    "EpubExtension::epub_creator",
                                                     info.creator);
            creator = info.creator;
            /* Cache the data so that we don't have to read it again */
            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_title",
                                            info.title);
            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_lang",
                                            info.lang);
            g_object_set_data(G_OBJECT (handle->file), 
                                            "EpubExtension::epub_creator",
                                            info.creator);
        } else {
            if(result == 1) {
                nautilus_file_info_add_string_attribute (handle->file,
                                                        "EpubExtension::epub_title",
                                                         info.title);
                g_object_set_data(G_OBJECT(handle->file), 
                                        "EpubExtension::epub_title",
                                        info.title);
                title = info.title;
            } else {
                char *data_s = g_strdup_printf("%s, Code: %d", epub_errors[result], result);
                nautilus_file_info_add_string_attribute (handle->file,
                                                        "EpubExtension::epub_title",
                                                         data_s);
                g_object_set_data(G_OBJECT(handle->file), 
                                        "EpubExtension::epub_title",
                                        data_s);
                g_free(data_s);
            }
        }
        g_free(filename);
    }
    /* Create GUI*/
    table = gtk_table_new(3, 2, TRUE);
    GtkWidget *labelL, labelC, labelT;
    GtkWidget *labelLd, labelCd, labelTd;
    labelL = gtk_label_new("Lang:");
    labelC = gtk_label_new("Creator:");
    labelT = gtk_label_new("Title:");
    labelLd = gtk_label_new(lang);
    labelCd = gtk_label_new(creator);
    labelTd = gtk_label_new(title);

    /* Table:
     0    1    2    3    5
    0+----+----+----+----+
     |    |    |    |    |
    1+----+----+----+----+
     |    |    |    |    |
    2+----+----+----+----+
     |    |    |    |    |
    3+----+----+----+----+

    gint left_attach,     x-координата левой верхней точки зацепления
    gint right_attach,    x-координата правой нижней точки зацепления
    gint top_attach,      y-координата левой верхней точки зацепления
    gint bottom_attach,   y-координата правой нижней точки зацепления
    */
    gtk_table_attach(GTK_TABLE (table), labelL, 0, 1, 0, 1,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE (table), labelC, 0, 1, 1, 2,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE (table), labelT, 0, 1, 2, 3,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);

    gtk_table_attach(GTK_TABLE (table), labelLd, 1, 2, 0, 1,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE (table), labelCd, 1, 2, 0, 1,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE (table), labelTd, 1, 2, 0, 1,
                    GTK_EXPAND, GTK_SHRINK, 0, 0);
    /* Add five pixels of spacing between every row and every column. */
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    return pages;
}
#endif
