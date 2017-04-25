To enable display of i965 batches for each render, apply the patch in
this directory to mesa:

0001-i965-batch-unconditionally-allocate-DEBUG_BATCH-hash.patch

After recompiling Mesa, ensure that LIBGL_DRIVERS_PATH points to the
directory containing i965_dri.so

This feature is a proof-of-concept.

