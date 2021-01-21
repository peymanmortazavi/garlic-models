nmap <Leader>b :make! -j8 -C build<CR>
nmap <Leader>n :make! -C build test<CR>
if $VIM_THEME != "EINK"
    colorscheme spacegray
    "colorscheme dracula
    let g:lightline.colorscheme = 'dracula'
endif
