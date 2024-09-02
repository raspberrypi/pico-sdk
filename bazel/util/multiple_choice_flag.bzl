def declare_flag_choices(flag, choices):
    """Declares a `config_setting` for each known choice for the provided flag.

    The name of each config setting uses the name of the `config_setting` is:
        [flag label name]_[choice]

    This can be used with select_choice() to map `config_setting`s to values.

    Args:
      flag: The flag that guides the declared `config_setting`s.
      pkg: The package that declare_flag_choices() was declared in.
      choice_map: A mapping of distinct choices to
    """
    flag_name = flag.split(":")[1]
    [
        native.config_setting(
            name = "{}_{}".format(flag_name, choice),
            flag_values = {flag: choice},
        )
        for choice in choices
    ]

def flag_choice(flag, pkg, choice_map):
    """Creates a `select()` based on choices declared by `declare_choices()`.

    Args:
      flag: The flag that guides the select.
      pkg: The package that `declare_flag_choices()` was called in.
      choice_map: A mapping of distinct choices to the final intended value.
    """
    return {
        "{}:{}_{}".format(
            pkg.split(":")[0],
            flag.split(":")[1],
            choice,
        ): val
        for choice, val in choice_map.items()
    }
