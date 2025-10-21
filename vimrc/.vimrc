""""""""""""""""""""""""""""""""""""""""""""
"基础配置
""""""""""""""""""""""""""""""""""""""""""""
"侦测文件类型
filetype on
"根据文件类型,加载相关插件,选择合适的缩进方式
filetype plugin indent on
"显示行号
set number
"语法高亮
syntax enable
syntax on
"tab键宽度
set tabstop=8
"不要用空格代替制表符
set noexpandtab
"统一缩进为8
set softtabstop=8
set shiftwidth=8
"显示制表符
set list lcs=tab:\|\ 
set list
"突出当前显示行
set cursorline
"自动缩进
set autoindent
set cindent
"为C程序提供自动缩进
set smartindent
"显示标尺
set ru
"高亮显示匹配的括号
set showmatch
"从不备份
set nobackup
"去掉vi一致性
set nocompatible
"自动保存
set autowrite
"搜索逐字符高亮
set hlsearch
"搜索时显示效果
set incsearch
"显示文件路径
set title
"当前光标行时钟在屏幕中间
set scrolloff=999
"历史计数
set history=1000
"显示文件状态栏
set laststatus=2
"设置256色，主题相关
set t_Co=256
"子目录中也可以使用C + ] 进行函数tag跳转
set tag=tags;/

"可以在buffer的任何地方使用鼠标
" set mouse=a
" set selection=exclusive
" set selectmode=mouse,key

"设置代码折叠方式
"set fdm=indent
"set fdm=marker
"set foldlevel=1

"80个字符分界方式1
"let &colorcolumn=join(range(81,999),",")
"hi ColorColumn ctermbg=black
"80个字符分界方式2
"set cc=81

""""""""""""""""""""""""""""""""""""""""""""
"键盘命令
""""""""""""""""""""""""""""""""""""""""""""
let mapleader = ","
noremap <leader>w :w<cr>
inoremap <leader>w <Esc>:w<cr>
"跳转到编号为 n buffer
noremap <leader>1 :b 1<cr>
noremap <leader>2 :b 2<cr>
noremap <leader>3 :b 3<cr>
noremap <leader>4 :b 4<cr>
noremap <leader>5 :b 5<cr>
noremap <leader>6 :b 6<cr>
noremap <leader>7 :b 7<cr>
noremap <leader>8 :b 8<cr>
noremap <leader>9 :b 9<cr>
"shift + 'char' 跳转到next/pre buff，删除buffer
noremap <S-k> :bn<cr>
noremap <S-j> :bp<cr>
noremap <leader>d :bd<cr>

"映射窗口移动快捷键<C-k,j,h,l>切换上下左右窗口
noremap <C-k> <C-w>k
noremap <C-j> <C-w>j
noremap <C-h> <C-w>h
noremap <C-l> <C-w>l

"jj进入normal模式 
inoremap jj <ESC>`^

"选中状态下leader+c复制/黏贴
noremap <leader>c "ay
noremap <leader>v "ap

"命令行模式增强，ctrl - a到行首，-e 到行尾
cnoremap <C-a> <Home>
cnoremap <C-e> <End>

"""""""""""""""""""""""""""""""""""""""""""""
"符号补全
"""""""""""""""""""""""""""""""""""""""""""""
:inoremap ' ''<ESC>i
:inoremap " ""<ESC>i
:inoremap ( ()<ESC>i
:inoremap ) <c-r>=ClosePair(')')<CR>
:inoremap [ []<ESC>i
:inoremap ] <c-r>=ClosePair(']')<CR>
:inoremap { {}<ESC>i<CR><ESC>O
:inoremap } <c-r>=ClosePair('}')<CR>

function! ClosePair(char)
	if getline('.')[col('.')-1] == a:char
		return "\<Right>"
	else
		return a:char
	endif
endfunction

" '' "" () {} []键跳出函数
func! SkipPair()
	if getline('.')[col('.') - 1] == ')' || getline('.')[col('.') - 1] == ']' || getline('.')[col('.') - 1] == '"' || getline('.')[col('.') - 1] == "'" || getline('.')[col('.') - 1] == '}'
		return "\<ESC>la"
	else
		return "\t"
	endif
endfunc

"将tab键绑定为跳出函数
inoremap <TAB> <c-r>=SkipPair()<CR>

"""""""""""""""""""""""""""""""""""""""""""
" 插件配置
"""""""""""""""""""""""""""""""""""""""""""
" startify
" 设置书签
let g:startify_bookmarks = ['~/.vimrc', '~/.bashrc', '~/.gitconfig']
let g:startify_files_numbers = 15
"let g:startify_lists = [
"	\{ 'type': 'dir',        'header': ['MRU'.getcwd()] },
"	\{ 'type': 'bookmarks',  'header': ['Bookmarks']    },
"	\{ 'type': 'commands',   'header': ['Commands']     },
"	\]

" Ctrlp
" ctrl + p 当前文件目录下查找
let g:ctrlp_map='<c-p>'
let g:ctrlp_cmd='CtrlP'
let g:ctrlp_working_path_mode='ca'

" shift + p MRU文件中查找
noremap <S-p> :CtrlPMRUFiles<cr>
let g:ctrlp_mruf_max=500
let g:ctrlp_max_depth=40

" airline
let g:airline_theme="hybrid"
" 打开minibuffer
let g:airline#extensions#tabline#enabled=1
let g:airline#extensions#tabline#formatter='unique_tail'
" buffer之间使用 | 分隔"
let g:airline#extensions#tabline#left_sep=''
let g:airline#extensions#tabline#left_alt_sep='|'
" 显示buffer编号
let g:airline#extensions#tabline#buffer_nr_show=1
" 状态栏关闭空格计数
let g:airline#extensions#whitespace#enabled=0

" NERDTree
let NERDTreeWinSize=30
let NERDTreeWinPos="left"
let NERDTreeShowHidden=1
let NERDTreeIgnore=['\.swp', '\.su', '\.o']
noremap <silent> <F2> :NERDTreeToggle<cr>
" leader + f快速在nerdtree中定位到当前文件
noremap <leader>f :NERDTreeFind<cr>
" Exit Vim if NERDTree is the only window left "
" autocmd BufEnter * if tabpagenr('$') == 1 && winnr('$') == 1 && exists('b:NERDTree') && b:NERDTree.isTabTree() | quit | endif
" Close the tab if NERDTree is the only window remaining in it.
autocmd BufEnter * if winnr('$') == 1 && exists('b:NERDTree') && b:NERDTree.isTabTree() | quit | endif

" tagbar
let g:tagbar_width=30
" 1：左边显示 0：右边显示
let g:tagbar_left=0
" tag缩进宽度为n<char>
let g:tagbar_indent=1
" 关闭排序,根据位置排序
let g:tagbar_sort=0
" 开启预览
" let g:tagbar_autopreview=1
noremap <leader>t :TagbarToggle<cr>

" fugitive
noremap <leader>g :Gstatus<cr>

" gitgutter
set updatetime=500
highlight GitGutterAdd ctermfg=green
highlight GitGutterChange ctermfg=yellow
highlight GitGutterDelete ctermfg=red
let g:gitgutter_max_signs = -1

" nerdcommenter
let g:NERDSpaceDelims=1
let g:NERDDefaultAlign='left'

" rainbow
let g:rainbow_active = 1

"""""""""""""""""""""""""""""""""""""""""""
" 主题配置
"""""""""""""""""""""""""""""""""""""""""""
set background=dark
colorscheme hybrid
" colorscheme gruvbox

