# Module Parsing

While there are plans to support JSON Schema and some other formats so
we are not reinventing the wheel and provide yet another format to learn
for model definitions. For those who prefer, there is this format which
some might find more intuitive to write.

This is still a work in progress but it is slowly taking shape.

In this document we use YAML to describe the format but you can use any data container.

Every module consists of fields and models. Fields are parsed before models regardless of
the order of fields in the module descriptor file.

Every field can either be an alias to another field (string) or define a Field structure (object).

```yaml
fields:
    FieldNameWithoutSpaces:
        meta:
            key1: value1
            key2: value2
        message: "This automatically ends up in the meta if defined."
        constraints: []  # empty constraints

    AnotherField:
        type: FieldNameWithoutSpaces  # inherit all constraints from this field.
        label: "Another Field"  # this goes to meta if defined.
        constraints:  # additional constraints
            - type: regex
              pattern: "your regex pattern here"
              name: "my_constraint"  # optional
              message: "My Custom Message"  # optional
              fatal: true  # also optional and defaults to false for regex

    DifferentName: AnotherField  # just an alias for AnotherField
```

Models are defined in a somewhat similar format. Each model can define `fields` which is an object
whose keys become field names and values become field descriptors. Either directly referring to a defined
field by its name in the module or making an anonymous field in-place.

```yaml
models:
    ModelNameWithoutSpaces:
        meta:
            key1: value1
            key2: value2
        description: "brief description"  # this goes into meta if defined.
        fields:
            username: DifferentName
            alternative_name: DifferentName?
            anonymous_field:
                type: string
                optional: true  # defaults to false
                constraints: [{type: range, min: 10, max: 25}]
```

As seen in the example above, you can refer to already defined fields like `username` key that uses `DifferentName`
defined earlier in the file. If you end the field name by `?` it makes that field optional.

Alternatively, you can define an anonymous field.

> **NOTE** Every module has **string**, **integer**, **double**, **bool**, **list**, **object** and **null** as fields by default.

\attention You cannot override fields and models.

### Automatic Fields
> Every model defined in the module automatically creates a field by the same name as the model.

```yaml
models:
    User:
        fields:
            name: string
            age: integer
    
    Group:
        fields:
            name: string
            users:
                constraints: [{type: list, of: User}]  # refers to User
```

### Deferred Fields

It is possible to use a field even prior to its definition so long as it gets defined somewhere in the module.

For example, we can improve the previous example by the following definition:

```yaml
fields:
    UserList:
        constraints: [{type: list, of: User}]  # User is not defined yet but it will be.

models:
    User:
        fields:
            name: string
            age: integer

    Group:
        fields:
            name: string
            users: UserList
```

This is the same for aliases, you can make a field alias for a field that is not defined yet.

### Model Inheritance

Models can inherit from one another. You can either inherit from one model or a series of models.

```yaml
models:
    User:
        fields:
            name: string

    AdminUser:
        inherit: User
        fields:
            level: integer

    GroupMembership:
        fields:
            group: string

    GroupAdmin:
        inherit: [GroupMembership, AdminUser]
```

#### Exclude some fields

You may exclude some fields from getting inherited.

```yaml
models:
    User:
        fields:
            name: string
            password: string

    UserInfo:
        inherit: User
        exclude_fields: [password]
```

#### Override fields

If a model defines a field, that will override the inheritance.

```yaml
models:
    User:
        fields:
            name: string
            age: integer

    YoungUser:
        inherit: User
        fields:
            age:
                constraints: [integer, {type: range, min: 5, max: 18}]
```

### ATTENTION

This format is very tentative and is still work in progress. There might be a change in the structure
so that fields don't have be explicitly under fields.

```yaml
models:
    User:
        @meta:
            key1: value1
        @inherit: []
        @exclude_fields: []
        name: string
        age: integer
```
