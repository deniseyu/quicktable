#include <Python.h>
#include "column.h"
#include "helpers.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

static int setup(void **state) {
  PyGILState_STATE *gstate;

  gstate = (PyGILState_STATE *)malloc(sizeof(PyGILState_STATE));
  *gstate = PyGILState_Ensure();

  *state = (void *)gstate;
  return 0;
}

static int teardown(void **state) {
  PyGILState_STATE *gstate;

  gstate = (PyGILState_STATE *)(*state);
  PyErr_Clear();
  PyGILState_Release(*gstate);
  free(*state);

  return 0;
}

static void test_qtb_column_new_fails(void **state) {
  QtbColumn *column;

  column = _qtb_column_new(&failing_malloc);
  assert_null(column);
}

static void test_qtb_column_new_many_fails(void **state) {
  QtbColumn *columns;

  columns = _qtb_column_new_many(20, &failing_malloc);
  assert_null(columns);
}

static void test_qtb_column_init_does_not_change_descriptor_refcount(void **state) {
  QtbColumn *column;
  PyObject *descriptor;
  Py_ssize_t descriptor_ref_count;

  column = qtb_column_new_succeeds();

  descriptor = new_descriptor("Name", "str");
  descriptor_ref_count = Py_REFCNT(descriptor);

  qtb_column_init(column, descriptor);
  qtb_column_dealloc(column);
  free(column);

  assert_int_equal(descriptor_ref_count, Py_REFCNT(descriptor));
  Py_DECREF(descriptor);
}

static void test_qtb_column_init_does_not_change_name_refcount(void **state) {
  QtbColumn *column;
  PyObject *descriptor;
  Py_ssize_t name_ref_count;

  column = qtb_column_new_succeeds();

  descriptor = new_descriptor("Name", "str");
  name_ref_count = Py_REFCNT(PyTuple_GET_ITEM(descriptor, 0));

  qtb_column_init(column, descriptor);
  qtb_column_dealloc(column);
  free(column);

  assert_int_equal(name_ref_count, Py_REFCNT(PyTuple_GET_ITEM(descriptor, 0)));
  Py_DECREF(descriptor);
}

static void test_qtb_column_init_does_not_change_type_refcount(void **state) {
  QtbColumn *column;
  PyObject *descriptor;
  Py_ssize_t type_ref_count;

  column = qtb_column_new_succeeds();

  descriptor = new_descriptor("Name", "str");
  type_ref_count = Py_REFCNT(PyTuple_GET_ITEM(descriptor, 1));

  qtb_column_init(column, descriptor);
  qtb_column_dealloc(column);
  free(column);

  assert_int_equal(type_ref_count, Py_REFCNT(PyTuple_GET_ITEM(descriptor, 1)));
  Py_DECREF(descriptor);
}

static void test_qtb_column_init_descriptor_not_sequence(void **state) {
  QtbColumn *column;
  bool result;

  column = qtb_column_new_succeeds();

  result = qtb_column_init(column, Py_None);
  qtb_column_dealloc(column);
  free(column);

  assert_false(result);
  assert_exc_string_equal("descriptor not a sequence");
}

static void test_qtb_column_init_strdup_fails(void **state) {
  QtbColumn *column;
  PyObject *descriptor;
  bool success;

  column = qtb_column_new_succeeds();
  column->strdup = &failing_strdup;

  descriptor = new_descriptor("Name", "str");

  success = qtb_column_init(column, descriptor);
  qtb_column_dealloc(column);
  free(column);
  Py_DECREF(descriptor);

  assert_false(success);
  assert_exc_string_equal("failed to initialise column");
}

static void test_qtb_column_init_free_on_fail(void **state) {
  QtbColumn *column;
  PyObject *descriptor;
  bool success;

  descriptor = new_descriptor("Name", "invalid");
  column = qtb_column_new_succeeds();

  success = qtb_column_init(column, descriptor);
  qtb_column_dealloc(column);
  free(column);
  Py_DECREF(descriptor);

  assert_false(success);
  assert_null(column->name);
  assert_exc_string_equal("invalid column type");
}

static const struct CMUnitTest tests[] = {
  cmocka_unit_test(test_qtb_column_new_fails),
  cmocka_unit_test(test_qtb_column_new_many_fails),

  cmocka_unit_test_setup_teardown(test_qtb_column_init_does_not_change_descriptor_refcount, setup, teardown),
  cmocka_unit_test_setup_teardown(test_qtb_column_init_does_not_change_name_refcount, setup, teardown),
  cmocka_unit_test_setup_teardown(test_qtb_column_init_does_not_change_type_refcount, setup, teardown),
  cmocka_unit_test_setup_teardown(test_qtb_column_init_descriptor_not_sequence, setup, teardown),
  cmocka_unit_test_setup_teardown(test_qtb_column_init_strdup_fails, setup, teardown),
  cmocka_unit_test_setup_teardown(test_qtb_column_init_free_on_fail, setup, teardown),
};

int test_column_run() {
  return cmocka_run_group_tests(tests, NULL, NULL);
}
