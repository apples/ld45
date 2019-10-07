Scripting
=========

Script Structure
----------------

A script must define the following functions:

### `update(eid, field)`

Will be called once per tick.

Parameters:

- `eid`: Entity ID of this entitiy.
- `field`: The game field.

Global Objects
--------------

These objects are always available to scripts.

### `entities`

The entity database.

Methods:

- `create_entity(net_id)`
- `destroy_entity(net_id)`
- `get_entity(net_id)`
- `create_component(ent_id, com)`
- `destroy_component(ent_id, com_type)`
- `get_component(ent_id, com_type)`
- `has_component(ent_id, com_type)`

### `direction`

Direction enum table.

Values:

- `NONE`
- `RP`
- `RN`
- `CP`
- `CN`

Global Functions
----------------

These functions are automatically provided to all scripts.

### `rotate_left(dir)`

Rotates a direction to the left.

Parameters:

- `dir`: The starting direction.

### `rotate_right(dir)`

Rotates a direction to the right.

Parameters:

- `dir`: The starting direction.

### `find_entities_at(entities, row, col)`

Gets a list of entities at the given position.

Parameters:

- `entities`: The entity database.
- `row`: Row.
- `col`: Col.

