from pathlib import Path
from os import system
import re, shutil

NEMU_HOME  = (Path(__file__) / '../../..').resolve()
EXPORT_DIR = NEMU_HOME / 'tools' / 'export' / 'output'
UNIFDEF_FLAGS = '-D__ICS_EXPORT -UISA64 -U__ENGINE_rv64__ -ULAZY_CC'
UNIFDEF_EXTRA_FILE_LIST = [
  r'^/runall.sh',
  r'^/Makefile.git',
]

def convert(r):
  return r.replace('.', r'\.').replace('*', r'[^/]*')

def list_filter(path, xs):
  for x in xs:
    if re.search(convert(x), path):
      return True
  return False

def export(white_list, black_list):
  def files():
    for abspath in NEMU_HOME.rglob('*'):
      if abspath.is_file():
        path = abspath.relative_to(NEMU_HOME)
        path_str = '/' + str(path)
        white = list_filter(path_str, white_list)
        black = list_filter(path_str, black_list)
        if white and not black:
          print('COPY', path)
          yield abspath, path
   
  try:
    shutil.rmtree(EXPORT_DIR)
  except:
    pass
  
  for abspath, relpath in files():
    src = abspath
    dst = EXPORT_DIR / relpath
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.match('*.[ch]') or list_filter('/' + str(relpath), UNIFDEF_EXTRA_FILE_LIST):
        system("unifdef " + UNIFDEF_FLAGS + ' ' + str(src) + " > " + str(dst))
    else:
        shutil.copyfile(src, dst)
