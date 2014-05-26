TEMPLATE = subdirs
SUBDIRS = CuteLogger mvcp src
cache()
src.depends = mvcp
