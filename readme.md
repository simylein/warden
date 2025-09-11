## warden home

a simple web application which presents iot device readings and metrics

### compile the source code

for development

```sh
make develop
```

for production

```sh
make release
```

### initialize the database

```sh
./warden --init
```

### populate the database

```sh
./warden --seed
```

### start the application

```sh
./warden
```

and enjoy ;^)

### configure the application

check out the available command line flags

```sh
./warden --help
```
