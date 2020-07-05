# Contribuir

## Codigo
Contribuir es muy sencillo una vez que te pones en ello, Usamos GitHub para nuestro repositorio online y rastrear los errores, asi que necesitaras una cuenta primero. Después de que hayas hecho eso y estés familiarizado de como la plataforma y git funcionan, Continua leyendo.

Para propósitos de esta guiá se usaran términos como: Master Branch (Rama principal) y Feature Branch (Rama de la característica a contribuir) 

### Prepara tu repositorio
1. Realiza un Fork de [LibreSprite](https://github.com/LibreSprite/LibreSprite) para que asi tengas tu repositorio personal para realizar 'push'.
2. Clona tu repositorio personal con `git clone --recursive https://github.com/TU-USUARIO/LibreSprite`. Recuerda cambiar el URL con el de tu repositorio. `--recursive` es usado para descargar las librerías de terceros para poder compilar el programa.
3. Dirígete a tu directorio, en linux esto se hace con el comando `cd LibreSprite`.
4. Sigue las [instrucciones](INSTALL_es.md) para compilar el programa y asegurarte que tu copia local funciona correctamente.
5. Agrega un 'upstream' remoto para que asi puedas obtener las actualizaciones de otros desarrolladores con `git remote add upstream https://github.com/LibreSprite/LibreSprite`.

### Actualiza tu repositorio
Estas instrucciones actualizaran ambos, tanto tu repositorio local como tu fork online.
1. `git fetch upstream` Descargara cualquier nuevo cambio desde el repositorio oficial de LibreSprite.
2. `git checkout master` Cambiara a tu rama master. 
3. `git merge upstream/master` fusionara los nuevos cambios a tu rama master local para que esta contenga todas las actualizaciones.
4. `git push origin master` Actualizara la rama master en tu repositorio online. Es una buena idea siempre mantenerla al dia.

### Realizando Cambios
Para hacer una contribución necesitaras crear una nueva rama para esta característica también (feature branch). Esto simplifica grandemente el añadir tu código al repositorio oficial y asegura que la rama master siempre funcione. Como aviso, siempre es bueno hacernos saber en que estas trabajando, comentando o creando un [issue](https://github.com/LibreSprite/LibreSprite/issues), De esta manera todos ahorramos tiempo trabajando en cosas diferentes.
1. Ejecuta el comando `git checkout master`, siempre asegurate que estés en tu rama master antes de crear una nueva rama.
2. Ejecuta el comando `git checkout -b nombre-de-tu-feature-branch`. Este comando creara una nueva rama  y cambiara a ella. Trata de ser especifico con el nombre de tu nueva feature branch, esto ayudara a encontrarla en un futuro si es necesario.
3. Haz un cambio significativo, Generalmente no quieres realizar la implementación de todo un set de nuevas características, Trata de separar tus tareas en pedazos pequeños pero significativos (y reversibles). A veces un cambio pequeño es todo lo que se necesita, como en la reparación de pequeños bugs.
4. Para crear un "commit", Empieza verificando con `git status` que solo los archivos que quieras cambiar sean modificados, Si hay algunos cambios inesperados, por favor resuelvelos antes de continuar.
5. Organiza todos tus cambios con `git add -A`.
6. Crea tu "commit" con `git commit -m "Escribe un mensaje con una descripción precisa de los cambios que has hecho."` Trata de describir los cambios como si lo hicieras en un registro de cambios. Mensajes como "Convertir miembros 'bool' en ui::Widget a flags" es lo que esperamos.
7. Repite los pasos 2-5 hasta que la característica que quieras implementar este completa.

### Pushing Cambios
Estas listo para que tu contribución sea revisada y aprobada. Hay algunas cosas que puedes hacer para hacer mas fácil la revision de tu código por parte de los moderadores. Asi que por favor sigue todos estos pasos.
1. Sigue las instrucciones para actualizar tu repositorio. (Ver arriba)
2. Cambia de nuevo a tu feature branch (la rama de tu nueva característica) con `git checkout nombre-de-tu-feature-branch`.
3. "Rebase" tu rama con `git rebase master`, esto se asegurara que no haya conflictos con las contribuciones de otras personas.
4. Recompila tu proyecto para asegurarte que todo sigue funcionando, Si las cosas ya no funcionan resuelvelas, haz comentarios conforme lo vayas haciendo. Una vez resueltas, vuelve al paso 1 ya que algunos cambios se pudieron haber hecho en ese tiempo.
5. "Push" tu rama a tu fork online con `git push origin nombre-de-tu-nueva-característica`.
6. En GitHub crea una pull request para la rama con tu contribución. 