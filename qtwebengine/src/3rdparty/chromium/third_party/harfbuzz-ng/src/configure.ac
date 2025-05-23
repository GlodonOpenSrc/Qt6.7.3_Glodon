AC_PREREQ([2.64])
AC_INIT([HarfBuzz],
        [7.3.0],
        [https://github.com/harfbuzz/harfbuzz/issues/new],
        [harfbuzz],
        [http://harfbuzz.org/])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/harfbuzz.pc.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.13.0 gnits tar-ustar dist-xz no-dist-gzip -Wall no-define color-tests -Wno-portability])
AM_SILENT_RULES([yes])
AX_CODE_COVERAGE

# Initialize libtool
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Check for programs
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(11)
AC_SYS_LARGEFILE
PKG_PROG_PKG_CONFIG([0.28])
AM_MISSING_PROG([RAGEL], [ragel])
AM_MISSING_PROG([GIT], [git])

# Version
m4_define(hb_version_triplet,m4_split(AC_PACKAGE_VERSION,[[.]]))
m4_define(hb_version_major,m4_argn(1,hb_version_triplet))
m4_define(hb_version_minor,m4_argn(2,hb_version_triplet))
m4_define(hb_version_micro,m4_argn(3,hb_version_triplet))
HB_VERSION_MAJOR=hb_version_major
HB_VERSION_MINOR=hb_version_minor
HB_VERSION_MICRO=hb_version_micro
HB_VERSION=AC_PACKAGE_VERSION
AC_SUBST(HB_VERSION_MAJOR)
AC_SUBST(HB_VERSION_MINOR)
AC_SUBST(HB_VERSION_MICRO)
AC_SUBST(HB_VERSION)

# Libtool version
m4_define([hb_version_int],
	  m4_eval(60000 + hb_version_major*100 + hb_version_minor*10 + hb_version_micro))
HB_LIBTOOL_VERSION_INFO=hb_version_int:0:hb_version_int
AC_SUBST(HB_LIBTOOL_VERSION_INFO)

AC_ARG_WITH([libstdc++],
	[AS_HELP_STRING([--with-libstdc++=@<:@yes/no@:>@],
			[Allow linking with libstdc++ @<:@default=no@:>@])],
	[with_libstdcxx=$withval],
	[with_libstdcxx=no])
AM_CONDITIONAL(WITH_LIBSTDCXX, [test "x$with_libstdcxx" = "xyes"])

# Documentation
have_gtk_doc=false
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.15],[--flavour no-tmpl])
	if test "x$enable_gtk_doc" = xyes; then
		have_gtk_doc=true
	fi
], [
	AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])

# Functions and headers
AC_CHECK_FUNCS(atexit mprotect sysconf getpagesize mmap isatty newlocale uselocale sincosf)
AC_CHECK_HEADERS(unistd.h sys/mman.h stdbool.h xlocale.h)

# Compiler flags
AC_CANONICAL_HOST
AC_CHECK_ALIGNOF([struct{char;}])
if test "x$GCC" = "xyes"; then

	# Make symbols link locally
	AX_CHECK_LINK_FLAG([[-Bsymbolic-functions]], [LDFLAGS="$LDFLAGS -Bsymbolic-functions"])

	# Make it possible to not link to libstdc++
	# No threadsafe statics in C++ as we do it ourselves.
	# We don't use these features, so it's safe to disable them
	# even in the cases where we DO link to libstdc++.
	# Put -fno-rtti before $CXXFLAGS such that users can re-enable it
	# by overriding CXXFLAGS.
	CXXFLAGS="-fno-rtti $CXXFLAGS -fno-exceptions -fno-threadsafe-statics"

	case "$host" in
		*-*-mingw*)
		;;
		*)
			# Hide inline methods
			CXXFLAGS="$CXXFLAGS -fvisibility-inlines-hidden"
		;;
	esac

	case "$host" in
		arm-*-*)
			if test "x$ac_cv_alignof_struct_char__" != x1; then
				# Request byte alignment
				CXXFLAGS="$CXXFLAGS -mstructure-size-boundary=8"
			fi
		;;
	esac
fi

AM_CONDITIONAL(HAVE_GCC, test "x$GCC" = "xyes")

hb_os_win32=no
AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    hb_os_win32=yes
    ;;
esac
AC_MSG_RESULT([$hb_os_win32])
AM_CONDITIONAL(OS_WIN32, test "$hb_os_win32" = "yes")

have_pthread=false
AX_PTHREAD([have_pthread=true])
if $have_pthread; then
	AC_DEFINE(HAVE_PTHREAD, 1, [Have POSIX threads])
fi
AM_CONDITIONAL(HAVE_PTHREAD, $have_pthread)

dnl ==========================================================================

AC_ARG_WITH(glib,
	[AS_HELP_STRING([--with-glib=@<:@yes/no/auto@:>@],
			[Use glib @<:@default=auto@:>@])],,
	[with_glib=auto])
have_glib=false
GLIB_DEPS="glib-2.0 >= 2.19.1"
AC_SUBST(GLIB_DEPS)
if test "x$with_glib" = "xyes" -o "x$with_glib" = "xauto"; then
	PKG_CHECK_MODULES(GLIB, $GLIB_DEPS, have_glib=true, :)
fi
if test "x$with_glib" = "xyes" -a "x$have_glib" != "xtrue"; then
	AC_MSG_ERROR([glib support requested but glib-2.0 not found])
fi
if $have_glib; then
	AC_DEFINE(HAVE_GLIB, 1, [Have glib2 library])
fi
AM_CONDITIONAL(HAVE_GLIB, $have_glib)

dnl ===========================================================================

AC_ARG_WITH(gobject,
	[AS_HELP_STRING([--with-gobject=@<:@yes/no/auto@:>@],
			[Use gobject @<:@default=no@:>@])],,
	[with_gobject=no])
have_gobject=false
if test "x$with_gobject" = "xyes" -o "x$with_gobject" = "xauto"; then
	PKG_CHECK_MODULES(GOBJECT, gobject-2.0 glib-2.0, have_gobject=true, :)
fi
if test "x$with_gobject" = "xyes" -a "x$have_gobject" != "xtrue"; then
	AC_MSG_ERROR([gobject support requested but gobject-2.0 / glib-2.0 not found])
fi
if $have_gobject; then
	AC_DEFINE(HAVE_GOBJECT, 1, [Have gobject2 library])
	GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
	AC_SUBST(GLIB_MKENUMS)
fi
AM_CONDITIONAL(HAVE_GOBJECT, $have_gobject)
AC_SUBST(have_gobject)

dnl ===========================================================================


dnl ===========================================================================
# Gobject-Introspection
have_introspection=false
m4_ifdef([GOBJECT_INTROSPECTION_CHECK], [
	if $have_gobject; then
		GOBJECT_INTROSPECTION_CHECK([1.34.0])
		if test "x$found_introspection" = xyes; then
			have_introspection=true
		fi
	else
		AM_CONDITIONAL([HAVE_INTROSPECTION], false)
	fi
], [
	AM_CONDITIONAL([HAVE_INTROSPECTION], false)
])

dnl ==========================================================================

AC_ARG_WITH(cairo,
	[AS_HELP_STRING([--with-cairo=@<:@yes/no/auto@:>@],
			[Use cairo @<:@default=auto@:>@])],,
	[with_cairo=auto])
have_cairo=false
if test "x$with_cairo" = "xyes" -o "x$with_cairo" = "xauto"; then
	PKG_CHECK_MODULES(CAIRO, cairo >= 1.8.0, have_cairo=true, :)
	save_libs=$LIBS
	LIBS="$LIBS $CAIRO_LIBS"
	AC_CHECK_FUNCS(cairo_user_font_face_set_render_color_glyph_func)
	LIBS=$save_libs
fi
if test "x$with_cairo" = "xyes" -a "x$have_cairo" != "xtrue"; then
	AC_MSG_ERROR([cairo support requested but not found])
fi
if $have_cairo; then
	AC_DEFINE(HAVE_CAIRO, 1, [Have cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO, $have_cairo)

have_cairo_ft=false
if $have_cairo; then
	PKG_CHECK_MODULES(CAIRO_FT, cairo-ft, have_cairo_ft=true, :)
fi
if $have_cairo_ft; then
	AC_DEFINE(HAVE_CAIRO_FT, 1, [Have cairo-ft support in cairo graphics library])
fi
AM_CONDITIONAL(HAVE_CAIRO_FT, $have_cairo_ft)

dnl ==========================================================================

AC_ARG_WITH(chafa,
	[AS_HELP_STRING([--with-chafa=@<:@yes/no/auto@:>@],
			[Use chafa @<:@default=auto@:>@])],,
	[with_chafa=auto])
have_chafa=false
if test "x$with_chafa" = "xyes" -o "x$with_chafa" = "xauto"; then
	PKG_CHECK_MODULES(CHAFA, chafa >= 1.6.0, have_chafa=true, :)
fi
if test "x$with_chafa" = "xyes" -a "x$have_chafa" != "xtrue"; then
	AC_MSG_ERROR([chafa support requested but not found])
fi
if $have_chafa; then
	AC_DEFINE(HAVE_CHAFA, 1, [Have chafa terminal graphics library])
fi
AM_CONDITIONAL(HAVE_CHAFA, $have_chafa)

dnl ==========================================================================

AC_ARG_WITH(icu,
	[AS_HELP_STRING([--with-icu=@<:@yes/no/builtin/auto@:>@],
			[Use ICU @<:@default=auto@:>@])],,
	[with_icu=auto])
have_icu=false
if test "x$with_icu" = "xyes" -o "x$with_icu" = "xbuiltin" -o "x$with_icu" = "xauto"; then
	PKG_CHECK_MODULES(ICU, icu-uc, have_icu=true, :)
fi
if test \( "x$with_icu" = "xyes" -o "x$with_icu" = "xbuiltin" \) -a "x$have_icu" != "xtrue"; then
	AC_MSG_ERROR([icu support requested but icu-uc not found])
fi

if $have_icu; then
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG --variable=CXXFLAGS icu-uc`"
	AC_DEFINE(HAVE_ICU, 1, [Have ICU library])
	if test "x$with_icu" = "xbuiltin"; then
		AC_DEFINE(HAVE_ICU_BUILTIN, 1, [Use hb-icu Unicode callbacks])
	fi
fi
AM_CONDITIONAL(HAVE_ICU, $have_icu)
AM_CONDITIONAL(HAVE_ICU_BUILTIN, $have_icu && test "x$with_icu" = "xbuiltin")

dnl ===========================================================================

AC_ARG_WITH(graphite2,
	[AS_HELP_STRING([--with-graphite2=@<:@yes/no/auto@:>@],
			[Use the graphite2 library @<:@default=no@:>@])],,
	[with_graphite2=no])
have_graphite2=false
GRAPHITE2_DEPS="graphite2 >= 1.2.0"
AC_SUBST(GRAPHITE2_DEPS)
if test "x$with_graphite2" = "xyes" -o "x$with_graphite2" = "xauto"; then
	PKG_CHECK_MODULES(GRAPHITE2, $GRAPHITE2_DEPS, have_graphite2=true, :)
	if test "x$have_graphite2" != "xtrue"; then
                # If pkg-config is not available, graphite2 can still be there
		ac_save_CFLAGS="$CFLAGS"
		ac_save_CPPFLAGS="$CPPFLAGS"
		CFLAGS="$CFLAGS $GRAPHITE2_CFLAGS"
		CPPFLAGS="$CPPFLAGS $GRAPHITE2_CFLAGS"
		AC_CHECK_HEADER(graphite2/Segment.h, have_graphite2=true, :)
		CPPFLAGS="$ac_save_CPPFLAGS"
		CFLAGS="$ac_save_CFLAGS"
	fi
fi
if test "x$with_graphite2" = "xyes" -a "x$have_graphite2" != "xtrue"; then
	AC_MSG_ERROR([graphite2 support requested but libgraphite2 not found])
fi
if $have_graphite2; then
	AC_DEFINE(HAVE_GRAPHITE2, 1, [Have Graphite2 library])
fi
AM_CONDITIONAL(HAVE_GRAPHITE2, $have_graphite2)

dnl ==========================================================================

AC_ARG_WITH(freetype,
	[AS_HELP_STRING([--with-freetype=@<:@yes/no/auto@:>@],
			[Use the FreeType library @<:@default=auto@:>@])],,
	[with_freetype=auto])
have_freetype=false
FREETYPE_DEPS="freetype2 >= 12.0.6"
AC_SUBST(FREETYPE_DEPS)
if test "x$with_freetype" = "xyes" -o "x$with_freetype" = "xauto"; then
	# See freetype/docs/VERSION.DLL; 12.0.6 means freetype-2.4.2
	PKG_CHECK_MODULES(FREETYPE, $FREETYPE_DEPS, have_freetype=true, :)
fi
if test "x$with_freetype" = "xyes" -a "x$have_freetype" != "xtrue"; then
	AC_MSG_ERROR([FreeType support requested but libfreetype2 not found])
fi
if $have_freetype; then
	AC_DEFINE(HAVE_FREETYPE, 1, [Have FreeType 2 library])
	save_libs=$LIBS
	LIBS="$LIBS $FREETYPE_LIBS"
	AC_CHECK_FUNCS(FT_Get_Var_Blend_Coordinates FT_Set_Var_Blend_Coordinates FT_Done_MM_Var FT_Get_Transform)
	LIBS=$save_libs
fi
AM_CONDITIONAL(HAVE_FREETYPE, $have_freetype)

dnl ===========================================================================

AC_ARG_WITH(uniscribe,
	[AS_HELP_STRING([--with-uniscribe=@<:@yes/no/auto@:>@],
			[Use the Uniscribe library @<:@default=no@:>@])],,
	[with_uniscribe=no])
have_uniscribe=false
if test "x$with_uniscribe" = "xyes" -o "x$with_uniscribe" = "xauto"; then
	AC_CHECK_HEADERS(usp10.h windows.h, have_uniscribe=true)
fi
if test "x$with_uniscribe" = "xyes" -a "x$have_uniscribe" != "xtrue"; then
	AC_MSG_ERROR([uniscribe support requested but not found])
fi
if $have_uniscribe; then
	UNISCRIBE_CFLAGS=
	UNISCRIBE_LIBS="-lusp10 -lgdi32 -lrpcrt4"
	AC_SUBST(UNISCRIBE_CFLAGS)
	AC_SUBST(UNISCRIBE_LIBS)
	AC_DEFINE(HAVE_UNISCRIBE, 1, [Have Uniscribe library])
fi
AM_CONDITIONAL(HAVE_UNISCRIBE, $have_uniscribe)

dnl ===========================================================================

AC_ARG_WITH(gdi,
	[AS_HELP_STRING([--with-gdi=@<:@yes/no/auto@:>@],
			[Provide GDI integration helpers @<:@default=no@:>@])],,
	[with_gdi=no])
have_gdi=false
if test "x$with_gdi" = "xyes" -o "x$with_gdi" = "xauto"; then
	AC_CHECK_HEADERS(windows.h, have_gdi=true)
fi
if test "x$with_gdi" = "xyes" -a "x$have_gdi" != "xtrue"; then
	AC_MSG_ERROR([gdi support requested but not found])
fi
if $have_gdi; then
	GDI_CFLAGS=
	GDI_LIBS="-lgdi32"
	AC_SUBST(GDI_CFLAGS)
	AC_SUBST(GDI_LIBS)
	AC_DEFINE(HAVE_GDI, 1, [Have GDI library])
fi
AM_CONDITIONAL(HAVE_GDI, $have_gdi)

dnl ===========================================================================

AC_ARG_WITH(directwrite,
	[AS_HELP_STRING([--with-directwrite=@<:@yes/no/auto@:>@],
			[Use the DirectWrite library (experimental) @<:@default=no@:>@])],,
	[with_directwrite=no])
have_directwrite=false
AC_LANG_PUSH([C++])
if test "x$with_directwrite" = "xyes" -o "x$with_directwrite" = "xauto"; then
	AC_CHECK_HEADERS(dwrite_1.h, have_directwrite=true)
fi
AC_LANG_POP([C++])
if test "x$with_directwrite" = "xyes" -a "x$have_directwrite" != "xtrue"; then
	AC_MSG_ERROR([directwrite support requested but not found])
fi
if $have_directwrite; then
	AC_DEFINE(HAVE_DIRECTWRITE, 1, [Have DirectWrite library])
fi
AM_CONDITIONAL(HAVE_DIRECTWRITE, $have_directwrite)

dnl ===========================================================================

AC_ARG_WITH(coretext,
	[AS_HELP_STRING([--with-coretext=@<:@yes/no/auto@:>@],
			[Use CoreText @<:@default=no@:>@])],,
	[with_coretext=no])
have_coretext=false
if test "x$with_coretext" = "xyes" -o "x$with_coretext" = "xauto"; then
	AC_CHECK_TYPE(CTFontRef, have_coretext=true,, [#include <ApplicationServices/ApplicationServices.h>])

	if $have_coretext; then
		CORETEXT_CFLAGS=
		CORETEXT_LIBS="-framework ApplicationServices"
		AC_SUBST(CORETEXT_CFLAGS)
		AC_SUBST(CORETEXT_LIBS)
	else
		# On iOS CoreText and CoreGraphics are stand-alone frameworks
		if test "x$have_coretext" != "xtrue"; then
			# Check for a different symbol to avoid getting cached result.
			AC_CHECK_TYPE(CTRunRef, have_coretext=true,, [#include <CoreText/CoreText.h>])
		fi

		if $have_coretext; then
			CORETEXT_CFLAGS=
			CORETEXT_LIBS="-framework CoreText -framework CoreGraphics -framework CoreFoundation"
			AC_SUBST(CORETEXT_CFLAGS)
			AC_SUBST(CORETEXT_LIBS)
		fi
	fi
fi
if test "x$with_coretext" = "xyes" -a "x$have_coretext" != "xtrue"; then
	AC_MSG_ERROR([CoreText support requested but libcoretext not found])
fi
if $have_coretext; then
	AC_DEFINE(HAVE_CORETEXT, 1, [Have Core Text backend])
fi
AM_CONDITIONAL(HAVE_CORETEXT, $have_coretext)

dnl ===========================================================================

AC_ARG_WITH(wasm,
	[AS_HELP_STRING([--with-wasm=@<:@yes/no/auto@:>@],
			[Use the wasm-micro-runtime library @<:@default=no@:>@])],,
	[with_wasm=no])
have_wasm=false
if test "x$with_wasm" = "xyes" -o "x$with_wasm" = "xauto"; then
	AC_CHECK_HEADERS(wasm_export.h, have_wasm=true)
fi
if test "x$with_wasm" = "xyes" -a "x$have_wasm" != "xtrue"; then
	AC_MSG_ERROR([wasm support requested but not found])
fi
if $have_wasm; then
	WASM_CFLAGS=
	WASM_LIBS="-liwasm"
	AC_SUBST(WASM_CFLAGS)
	AC_SUBST(WASM_LIBS)
	AC_DEFINE(HAVE_WASM, 1, [Have wasm-micro-runtime library])
fi
AM_CONDITIONAL(HAVE_WASM, $have_wasm)

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
src/Makefile
src/harfbuzz-config.cmake
util/Makefile
test/Makefile
test/api/Makefile
test/fuzzing/Makefile
test/shape/Makefile
test/shape/data/Makefile
test/shape/data/aots/Makefile
test/shape/data/in-house/Makefile
test/shape/data/text-rendering-tests/Makefile
test/subset/Makefile
test/subset/data/Makefile
test/subset/data/repack_tests/Makefile
test/threads/Makefile
perf/Makefile
docs/Makefile
docs/version.xml
])

AC_OUTPUT

echo
echo "C++ compiler version:"
$CXX --version
echo

AC_MSG_NOTICE([

Autotools is no longer our supported build system for building the library
for *nix distributions, please migrate to meson.

])


AC_MSG_NOTICE([

Build configuration:

Unicode callbacks (you want at least one):
	Builtin			true
	Glib:			${have_glib}
	ICU:			${have_icu}

Font callbacks (the more the merrier):
	FreeType:		${have_freetype}

Tools used for command-line utilities:
	Cairo:			${have_cairo}
	Chafa:			${have_chafa}

Additional shapers:
	Graphite2:		${have_graphite2}

Platform shapers (not normally needed):
	CoreText:		${have_coretext}
	DirectWrite:		${have_directwrite}
	GDI:			${have_gdi}
	Uniscribe:		${have_uniscribe}
	WebAssembly:		${have_wasm}

Other features:
	Documentation:		${enable_gtk_doc}
	GObject bindings:	${have_gobject}
	Introspection:		${have_introspection}
])
