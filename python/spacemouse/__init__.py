import blue
import sys

# This is a hack to allow PyCharm to parse stub files for _spacemouse. The _spacemouse_stub stub is located
# in packages/stubgen/stubs and will always generate an ImportError.
try:
    from _spacemouse_stub import *
except ImportError:
    pass

sys.modules[__name__] = blue.LoadExtension("_spacemouse")
