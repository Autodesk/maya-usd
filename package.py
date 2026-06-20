name = "maya_usd"

version = "0.36.0.hh.1.0.0"

authors = [
    "Autodesk",
]

description = """Maya USD plugin"""

with scope("config") as c:
    import os

    c.release_packages_path = os.environ["HH_REZ_REPO_RELEASE_EXT"]

requires = []

private_build_requires = [
    # "vulkanSDK",
    "PyOpenGL",
    "Jinja2",
    "PySide6",
    "PyYAML",
]

variants = [
    ["maya-2026.3", "usd-25.08", "python-3.11.9"],  # Maya 2026: undefined symbol _PyModule_add issue
]


def commands():
    env.REZ_MAYA_USD_ROOT = "{root}"
    env.MAYA_USD_ROOT = "{root}"
    env.MAYA_USD_LOCATION = "{root}"


def post_commands():
    # NOTE: We prepend (so Maya finds it first) here (post_commands) so the Maya's
    # package (hh_rez_maya) looses the race.
    env.MAYA_MODULE_PATH.prepend("{root}")


uuid = "repository.maya-usd"