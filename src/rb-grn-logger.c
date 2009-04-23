/* -*- c-file-style: "ruby" -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "rb-grn.h"

#define RVAL2GRNWRAPPER(object)  (rb_grn_logger_info_wrapper_from_ruby_object(object))
#define RVAL2GRNLOGLEVEL(object) (rb_grn_log_level_from_ruby_object(object))

VALUE cGrnLogger;

typedef struct _rb_grn_logger_info_wrapper
{
    grn_logger_info *logger;
    VALUE handler;
} rb_grn_logger_info_wrapper;

static rb_grn_logger_info_wrapper *
rb_grn_logger_info_wrapper_from_ruby_object (VALUE object)
{
    rb_grn_logger_info_wrapper *wrapper;

    if (NIL_P(object))
        return NULL;

    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, cGrnLogger))) {
	rb_raise(rb_eTypeError, "not a groonga logger");
    }

    Data_Get_Struct(object, rb_grn_logger_info_wrapper, wrapper);
    if (!wrapper)
        rb_raise(rb_eTypeError, "groonga logger is NULL");

    return wrapper;
}

grn_logger_info *
rb_grn_logger_from_ruby_object (VALUE object)
{
    rb_grn_logger_info_wrapper *wrapper;

    wrapper = RVAL2GRNWRAPPER(object);
    if (!wrapper)
        return NULL;

    return wrapper->logger;
}

static void
rb_grn_logger_free (void *object)
{
    rb_grn_logger_info_wrapper *wrapper = object;

    xfree(wrapper->logger);
    xfree(wrapper);
}

static VALUE
rb_grn_logger_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, rb_grn_logger_free, NULL);
}

static grn_log_level
rb_grn_log_level_from_ruby_object (VALUE rb_level)
{
    grn_log_level level = GRN_LOG_NONE;

    if (NIL_P(rb_level)) {
        level = GRN_LOG_DEFAULT_LEVEL;
    } else if (rb_grn_equal_option(rb_level, "none")) {
        level = GRN_LOG_NONE;
    } else if (rb_grn_equal_option(rb_level, "emergency")) {
        level = GRN_LOG_EMERG;
    } else if (rb_grn_equal_option(rb_level, "alert")) {
        level = GRN_LOG_ALERT;
    } else if (rb_grn_equal_option(rb_level, "critical")) {
        level = GRN_LOG_CRIT;
    } else if (rb_grn_equal_option(rb_level, "error")) {
        level = GRN_LOG_ERROR;
    } else if (rb_grn_equal_option(rb_level, "warning")) {
        level = GRN_LOG_WARNING;
    } else if (rb_grn_equal_option(rb_level, "notice")) {
        level = GRN_LOG_NOTICE;
    } else if (rb_grn_equal_option(rb_level, "info")) {
        level = GRN_LOG_INFO;
    } else if (rb_grn_equal_option(rb_level, "debug")) {
        level = GRN_LOG_DEBUG;
    } else if (rb_grn_equal_option(rb_level, "dump")) {
        level = GRN_LOG_DUMP;
    } else {
        rb_raise(rb_eArgError,
                 "log level should be one of "
                 "[nil, :none, :emergency, :alert, :critical, :error, "
                 ":warning, :notice, :info, :debug, :dump]: %s",
                 rb_grn_inspect(rb_level));
    }

    return level;
}

static void
rb_grn_log (int level, const char *time, const char *title,
            const char *message, const char *location, void *func_arg)
{
    rb_grn_logger_info_wrapper *wrapper = func_arg;

    rb_funcall(wrapper->handler, rb_intern("call"), 5,
               INT2NUM(level),
               rb_str_new2(time),
               rb_str_new2(title),
               rb_str_new2(message),
               rb_str_new2(location));
}

static void
rb_grn_logger_set_handler (VALUE self, VALUE rb_handler)
{
    rb_grn_logger_info_wrapper *wrapper;
    grn_logger_info *logger;

    wrapper = RVAL2GRNWRAPPER(self);
    wrapper->handler = rb_handler;
    rb_iv_set(self, "@handler", rb_handler);

    logger = wrapper->logger;
    if (NIL_P(rb_handler)) {
        logger->func = NULL;
        logger->func_arg = NULL;
    } else {
        logger->func = rb_grn_log;
        logger->func_arg = wrapper;
    }
}

static VALUE
rb_grn_logger_initialize (int argc, VALUE *argv, VALUE self)
{
    rb_grn_logger_info_wrapper *wrapper;
    grn_logger_info *logger;
    grn_log_level level;
    int flags = 0;
    VALUE options, rb_level, rb_time, rb_title, rb_message, rb_location;
    VALUE rb_handler;

    rb_scan_args(argc, argv, "01&", &options, &rb_handler);

    rb_grn_scan_options(options,
                        "level", &rb_level,
                        "time", &rb_time,
                        "title", &rb_title,
                        "message", &rb_message,
                        "location", &rb_location,
                        NULL);

    level = RVAL2GRNLOGLEVEL(rb_level);

    if (NIL_P(rb_time) || RVAL2CBOOL(rb_time))
        flags |= GRN_LOG_TIME;
    if (NIL_P(rb_title) || RVAL2CBOOL(rb_title))
        flags |= GRN_LOG_TITLE;
    if (NIL_P(rb_message) || RVAL2CBOOL(rb_message))
        flags |= GRN_LOG_MESSAGE;
    if (NIL_P(rb_location) || RVAL2CBOOL(rb_location))
        flags |= GRN_LOG_LOCATION;

    wrapper = ALLOC(rb_grn_logger_info_wrapper);
    logger = ALLOC(grn_logger_info);
    wrapper->logger = logger;
    DATA_PTR(self) = wrapper;

    logger->max_level = level;
    logger->flags = flags;
    rb_grn_logger_set_handler(self, rb_handler);

    return Qnil;
}

static VALUE
rb_grn_logger_s_register (int argc, VALUE *argv, VALUE klass)
{
    VALUE logger;
    grn_rc  rc;

    logger = rb_funcall2(klass, rb_intern("new"), argc, argv);
    rb_grn_logger_set_handler(logger, rb_block_proc());
    rc = grn_logger_info_set(NULL, RVAL2GRNLOGGER(logger));
    rb_grn_rc_check(rc, logger);
    rb_cv_set(klass, "@@current_logger", logger);

    return Qnil;
}

static void
rb_grn_logger_reset (VALUE klass)
{
    VALUE current_logger;

    current_logger = rb_cv_get(klass, "@@current_logger");
    if (NIL_P(current_logger))
        return;

    grn_logger_info_set(NULL, NULL);
}

void
rb_grn_init_logger (VALUE mGrn)
{
    cGrnLogger = rb_define_class_under(mGrn, "Logger", rb_cObject);
    rb_define_alloc_func(cGrnLogger, rb_grn_logger_alloc);

    rb_cv_set(cGrnLogger, "@@current_logger", Qnil);
    rb_define_singleton_method(cGrnLogger, "register",
                               rb_grn_logger_s_register, -1);
    rb_set_end_proc(rb_grn_logger_reset, cGrnLogger);

    rb_define_method(cGrnLogger, "initialize", rb_grn_logger_initialize, -1);
}