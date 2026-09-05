# Prodot Engine

<p align="center">
  <a href="https://godotengine.org">
    <img src="misc/logo/logo_outlined.png" width="400" alt="Godot Engine logo">
  </a>
</p>

## A simple fork testing some usability improvements

Modifications currently built on top of Godot 4.7.2.

### Currently testing:

- The top workspace buttons (`2D`/`3D`/`Script`/`Game`/`Asset Store`) have been changed to `Scene`/`Script`/`Game`/`Asset Store` and now switch between different sets of tabs.\
No longer will it always be scene tabs with possibly unrelated content below.
- `2D`/`3D` is now a view option selectable on the scene toolbar and it will automatically choose the correct mode when making new scenes or opening existing (based on the root node type).
- The script editor now has its own tabs when in `Script` mode.

### Minor improvements:
- Opening scene files now automatically switches back to the `Scene` workspace so you can view it (previously it would stay in script view or otherwise)
- The inspector now provides more room for settings when enlarged (previously it was always 50% label, resulting in a lot of empty white space when large)
