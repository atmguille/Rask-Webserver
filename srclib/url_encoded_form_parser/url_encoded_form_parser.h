#ifndef URL_ENCODED_FORM_PARSER
#define URL_ENCODED_FORM_PARSER

typedef struct _Form Form;

Form *form_ini(const char *body, int body_size);
void form_print(Form *form);

#endif // URL_ENCODED_FORM_PARSER
