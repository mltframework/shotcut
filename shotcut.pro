TEMPLATE = subdirs
SUBDIRS = CuteLogger mvcp src translations
cache()
src.depends = CuteLogger mvcp
