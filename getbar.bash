# bash completion for getbar

_getbar()
{
	local cur prev words cword
	_init_completion || return

	if [[ $cur == -* ]]; then
        COMPREPLY=($(compgen -W '-b -i -s -w -p -g -f -e -c -d -v -q -h --block-size --interval --size-max --window --polynomial --gnuplot --force --estimate-bps --count --delayed --verbose --quiet --help --version' -- "$cur"))
		return
	fi

	_comp_compgen_userland
}

complete -F _getbar getbar
