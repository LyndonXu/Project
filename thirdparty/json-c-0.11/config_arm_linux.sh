#! /bin/sh
rm arm-linux.cache
touch arm-linux.cache
echo ac_cv_func_malloc_0_nonnull=\${ac_cv_func_malloc_0_nonnull=yes} >> arm-linux.cache
echo ac_cv_func_realloc_0_nonnull=\${ac_cv_func_realloc_0_nonnull=yes} >> arm-linux.cache

