#ifndef _NAUTILUS_EXTENSION_EPUB_
#define _NAUTILUS_EXTENSION_EPUB_

void nautilus_module_initialize(GTypeModule *module);
void nautilus_module_shutdown(void);
void nautilus_module_list_types(const GType **types, int *num_types);
static void epub_extension_column_provider_iface_init(
                                NautilusColumnProviderIface *iface);
static void epub_extension_info_provider_iface_init(
                                NautilusInfoProviderIface *iface);
static void epub_extension_register_type(GTypeModule *module);
GType epub_extension_get_type(void);


static void epub_extension_class_init    (EpubExtensionClass *class);
static void epub_extension_instance_init (EpubExtension      *img);
static GList *epub_extension_get_columns (NautilusColumnProvider *provider);
static void epub_extension_cancel_update (NautilusInfoProvider *provider,
                                          NautilusOperationHandle *handle);
static NautilusOperationResult epub_extension_update_file_info (
                                NautilusInfoProvider *provider,
                                NautilusFileInfo *file,
                                GClosure *update_complete,
                                NautilusOperationHandle **handle);

gint timeout_epub_callback(gpointer data);

/* ------- Start Epub only */
static const int MAX_STR_LEN = 256;
static const int ZIP_BUFFER_LEN = 1024;

 enum FSM_State {
    INIT,
    BOOK_TITLE_OPENED,
    BOOK_TITLE_END,
    CREATOR_OPENED,
    CREATOR_END,
    LANG_OPENED,
    LANG_END,
    STOP
};

static const int LANG_LENGTH = 6;
typedef struct {
    char title[MAX_STR_LEN];
    char creator[MAX_STR_LEN];
    char lang[LANG_LENGTH];
    enum FSM_State my_state;
} EpubInfo;
        
static int read_from_epub(const char *archive, EpubInfo *info);
static int parse_xml_from_buffer(char *content, zip_uint64_t uncomp_size, EpubInfo *info);

static void make_sax_handler_container(xmlSAXHandler *SAXHander, void *user_data);
static void make_sax_handler_contentOPF(xmlSAXHandler *SAXHander, void *user_data);
static void OnStartElementNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
    );
static void OnStartElementContainerNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
    );

static void OnEndElementNs(
    void* ctx,
    const xmlChar* localname,
    const xmlChar* prefix,
    const xmlChar* URI
    );

static void OnCharacters(void *ctx, const xmlChar *ch, int len);

const static char *epub_errors[] = {"ok", "ZIP read error", "ZIP inner file open error",
                                    "Epub container.xml file parse XML error",
                                    "Epub OPF file parse XML error",
                                    "can't close zip archive"};

static void my_strlcpy(char *dest, const char *src_begin, const char *src_end, size_t count);
static int my_strncmp(const char *lhs, const char *rhs_begin, const char *rhs_end, size_t count);

#endif /* _NAUTILUS_EXTENSION_EPUB_ */
