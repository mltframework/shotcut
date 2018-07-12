TEMPLATE = subdirs
SUBDIRS = CuteLogger src translations
cache()
src.depends = CuteLogger
