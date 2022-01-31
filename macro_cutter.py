from typing import Optional, List, Iterator, Union, Tuple, NamedTuple
import logging
import pathlib
import sys
import re
logger = logging.getLogger(__name__)
MACRO = re.compile(r'#\s*(\w+)(.*)')


class Macro(NamedTuple):
    macro: str
    arg: str

    def __str__(self) -> str:
        return f'[#{self.macro} {self.arg}]'


class Scope:
    def __init__(self, start: int, defines: List[str], macro: Optional[Macro] = None) -> None:
        self.start = start+1
        self.defines = list(defines)
        self.macros = [macro]
        #
        self.lines: List[Union[str, Scope]] = []
        self.end = None
        logger.debug(f'enter: {self}')

    def __str__(self) -> str:
        return f'{", ".join(str(m) for m in self.macros)}: {self.start} to {self.end}'

    def push(self, end: Optional[int] = None, l: Union[str, 'Scope', None] = None):
        if isinstance(end, int) and l != None:
            self.end = end + 1
            self.lines.append(l)
        else:
            logger.debug(f'close: {self}')

    def parse(self, it: Iterator[Tuple[int, str]]):
        # copy
        defines = list(self.defines)
        while True:
            try:
                i, l = next(it)
                m = MACRO.match(l)
                if m:
                    match m.group(1):
                        case 'ifdef':
                            child = Scope(i, defines, Macro(m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'ifndef':
                            child = Scope(i, defines, Macro(m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'if':
                            child = Scope(i, defines, Macro(m.group(1),
                                          m.group(2).strip()))
                            self.push(i, child)
                            child.parse(it)
                        case 'elif':
                            self.macros.append(
                                Macro(m.group(1), m.group(2).strip()))
                            self.push(i, l)
                        case 'else':
                            self.macros.append(
                                Macro(m.group(1), ''))
                            self.push(i, l)
                        case 'endif':
                            self.push(i, l)
                            return
                        case 'define':
                            self.push(i, l)
                            defines.append(m.group(2).strip())
                        case 'include':
                            self.push(i, l)
                        case _:
                            self.push(i, l)
                else:
                    self.push(i, l)

            except StopIteration:
                break

    def print(self, level=0):
        indent = '  ' * level
        print(f'{indent}{self}')
        for child in self.lines:
            match child:
                case Scope() as scope:
                    scope.print(level+1)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    text = pathlib.Path(sys.argv[1]).read_text()
    root = Scope(0, [])
    root.parse(enumerate(text.splitlines()))
    root.print()
