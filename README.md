Pour débugger : `make gdb` dans un premier terminal et `make debug` dans un second terminal.
Dans `gdb` on utilise `monitor help` pour voir les commandes `qemu` disponibles et `monitor <cmd>` pour exécuter ces commandes.
La commande `monitor gdbserver none` permet de couper la connexion entre `qemu` et `gdb`.
