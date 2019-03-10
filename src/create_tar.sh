#!/bin/bash

# \file create_tar.sh  
# \author Lorenzo Boccolini 544098
# Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
# originale dell'autore
    
#controllo che lo script sia stato lanciato con almeno 2 argomenti
if [ $# -lt 2 ]; then
	echo "use: $(basename $0) conf_file n_minutes [--help] "
	exit 1
fi

#controllo che lo script non sia stato lanciato con troppi argomenti
if [ $# -gt 3 ]; then
	echo "use: $(basename $0) conf_file n_minutes [--help] "
	exit 1
fi

#se lo script è stato lanciato con tre argomenti e il terzo è '--help',stampo lo usage
if [ $# -eq 3 ]; then
	if [ $3 -eq "--help"]; then
		echo "use: $(basename $0) conf_file n_minutes [--help] "
	fi
fi

#controllo che il file passato come primo argomento sia un file regolare
if [ ! -f $1 ]; then
	echo "Il file '$1' non esiste o non è un file regolare"
	exit 1
fi

#controllo che il parametro n_minutes sia non negativp
if [ $2 -lt 0 ]; then
	echo "Il parametro 'n_minutes' deve ssere >=0 "
	exit 1
fi

#cerco "DirName" nel file, che sarà seguito da un '=', e dal nome della directory
trovata=0 
while read -r line && [ ! $trovata -eq 3 ]
do
	for word in $line
	do
		if [ $trovata -eq 0 ]; then
			if [ $word == "DirName" ]; then
				((trovata=1))
			fi
		elif [ $trovata -eq 1 ]; then
				((trovata=2))
		else
				dir_name=$word
				((trovata=3))
		fi
	done
done < $1

if [ $dir_name ]; then
    #mi sposto nella cartella in cui lavorare
    cd $dir_name
    #se n_minutes è diverso da 2, creo il tar.gz ed elimino i files
    if [ ! $2 -eq 0 ]; then
	find -type f -mmin +$2 -exec tar -zcf chatty_archive.tar.gz $dir_name \;
	if [ $? -eq 0  ]; then
	    find -type f -mmin +$2 -exec rm {} \;
	else
	    exit 1
	fi
    #se n_minutes era 0, stampo i file senza creare il tar.gz e senza eliminare
    else
	find -type f
    fi
#se dir_name non è definito, non ho trovato 'DirName = ... ' nel file passato come argomento
else 
	echo "Il file '$1' non contiene un parametro valido per 'DirName'"
	exit 1
fi
exit 0 
