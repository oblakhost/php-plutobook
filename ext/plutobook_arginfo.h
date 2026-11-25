/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 93993498521e629fd29f3a4f5245ea5afdb9a612 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_PlutoBook_Document___construct, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pageSize, IS_STRING, 0, "\"A4\"")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, orientation, IS_STRING, 0, "\"portrait\"")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, margins, IS_STRING, 0, "\"15mm\"")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PlutoBook_Document_loadHtml, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, html, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, baseUrl, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, css, IS_STRING, 0, "\"\"")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, documentRoot, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PlutoBook_Document_toPdf, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PlutoBook_Document_writeToStream, 0, 1, IS_VOID, 0)
	ZEND_ARG_INFO(0, stream)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PlutoBook_Document_writeToFile, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PlutoBook_Document_getResourceErrors, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()


ZEND_METHOD(PlutoBook_Document, __construct);
ZEND_METHOD(PlutoBook_Document, loadHtml);
ZEND_METHOD(PlutoBook_Document, toPdf);
ZEND_METHOD(PlutoBook_Document, writeToStream);
ZEND_METHOD(PlutoBook_Document, writeToFile);
ZEND_METHOD(PlutoBook_Document, getResourceErrors);


static const zend_function_entry class_PlutoBook_Exception_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_PlutoBook_Document_methods[] = {
	ZEND_ME(PlutoBook_Document, __construct, arginfo_class_PlutoBook_Document___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(PlutoBook_Document, loadHtml, arginfo_class_PlutoBook_Document_loadHtml, ZEND_ACC_PUBLIC)
	ZEND_ME(PlutoBook_Document, toPdf, arginfo_class_PlutoBook_Document_toPdf, ZEND_ACC_PUBLIC)
	ZEND_ME(PlutoBook_Document, writeToStream, arginfo_class_PlutoBook_Document_writeToStream, ZEND_ACC_PUBLIC)
	ZEND_ME(PlutoBook_Document, writeToFile, arginfo_class_PlutoBook_Document_writeToFile, ZEND_ACC_PUBLIC)
	ZEND_ME(PlutoBook_Document, getResourceErrors, arginfo_class_PlutoBook_Document_getResourceErrors, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_PlutoBook_Exception(zend_class_entry *class_entry_RuntimeException)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "PlutoBook", "Exception", class_PlutoBook_Exception_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RuntimeException);

	return class_entry;
}

static zend_class_entry *register_class_PlutoBook_Document(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "PlutoBook", "Document", class_PlutoBook_Document_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}
