#! /usr/bin/env python3
'''
CUI tool to interact with OBS Studio through obs-websocket
'''

import argparse
import json
import textwrap
import obsws_python

def _is_simple(data):
    return isinstance(data, (str, bool, int, float))

def _pretty_format(data):
    # pylint: disable=too-many-return-statements
    if isinstance(data, dict):
        ff_simple = []
        ff_compex = []
        has_complex = False
        for key, value in data.items():
            t = f'"{key}": {_pretty_format(value)}'
            if _is_simple(value):
                ff_simple.append(t)
            else:
                ff_compex.append(t)
        if not ff_simple and not ff_compex:
            return '{}'
        if not ff_compex:
            return '{' + ', '.join(ff_simple) + '}'
        if not ff_simple:
            return '{\n' + textwrap.indent(',\n'.join(ff_compex), '  ') + '\n}'
        return '{\n  ' + \
                ', '.join(ff_simple) + ',\n' + \
                textwrap.indent(',\n'.join(ff_compex), '  ') + \
                '\n}'
    if isinstance(data, list):
        ff = []
        has_complex = False
        for value in data:
            if not _is_simple(value):
                has_complex = True
            ff.append(_pretty_format(value))
        if not ff:
            return '[]'
        if not has_complex:
            return '[' + ', '.join(ff) + ']'
        return '[\n' + textwrap.indent(',\n'.join(ff), '  ') + '\n]'

    return json.dumps(data, indent=2)

def _can_remove(key, value):
    # pylint: disable=too-many-return-statements
    if _is_simple(value):
        if key in ('visible', 'enabled'):
            if value:
                return True
        elif not value:
            return True
        if key == 'toolTipDuration' and value == -1:
            return True
        if key == 'windowOpacity' and value == 1.0:
            return True
        if key == 'updatesEnabled' and value:
            return True
        if key in ('maximumWidth', 'maximumHeight') and value == 0xFFFFFF:
            return True
        if key in ('autoRepeat', 'autoRepeatDelay', 'autoRepeatInterval'):
            return True
    if value == []:
        return True
    return False

def _remove_empty(data):
    if isinstance(data, dict):
        remove_keys = []
        for key, value in data.items():
            if _can_remove(key, value):
                remove_keys.append(key)
            elif not _is_simple(value):
                _remove_empty(value)
        for key in remove_keys:
            del data[key]
    if isinstance(data, list):
        for value in data:
            if not _is_simple(value):
                _remove_empty(value)
    return data

def _remove_by_key(data, keys):
    if isinstance(data, dict):
        for key in keys:
            if key in data:
                del data[key]
        for value in data.values():
            _remove_by_key(value, keys)
    elif isinstance(data, list):
        for value in data:
            _remove_by_key(value, keys)

def _filter_tree_match(data, cond):
    for key, value in cond.items():
        if key not in data:
            return False
        if data[key] != value:
            return False
    return True

def _filter_tree(data, children_key, path, i_path=0):
    if not isinstance(data, list):
        return data

    ret = []
    for value in data:
        if not isinstance(value, dict):
            ret.append(value)
            continue

        if i_path < len(path) and not _filter_tree_match(value, path[i_path]):
            continue
        if i_path + 1 < len(path):
            children = _filter_tree(value[children_key], children_key, path, i_path + 1)
            if not children:
                continue
            value = dict(value)
            value[children_key] = children
        ret.append(value)
    return ret

def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--list-menu', action='store_true', default=False)
    parser.add_argument('--list-widgets', action='store_true', default=False)
    parser.add_argument('--trigger', action='store_true', default=None)
    parser.add_argument('--invoke-widget', action='store_true', default=None)
    parser.add_argument('--simplify', action='store_true', default=False)
    parser.add_argument('--no-geometry', action='store_true', default=False)
    parser.add_argument('path', nargs='?', default=None)
    parser.add_argument('args', nargs='*', default=[])
    args = parser.parse_args()

    if args.path:
        args.path = json.loads(args.path)

    return args

def _main():
    args = _get_args()

    cl = obsws_python.ReqClient(host='localhost', port=4455)

    if args.list_menu:
        res = cl.send('CallVendorRequest', {
            'vendorName': 'ui-ws-automation',
            'requestType': 'menu-list',
        })
        data = res.response_data['menu']
        if args.path:
            data = _filter_tree(data, 'menu', args.path)
        if args.simplify:
            data = _remove_empty(data)
        print(_pretty_format(data))

    if args.list_widgets:
        res = cl.send('CallVendorRequest', {
            'vendorName': 'ui-ws-automation',
            'requestType': 'widget-list',
        })
        data = res.response_data['children']
        if args.path:
            data = _filter_tree(data, 'children', args.path)
        if args.simplify:
            _remove_empty(data)
        if args.no_geometry:
            _remove_by_key(data, (
                'width', 'height', 'minimumWidth', 'minimumHeight', 'maximumWidth', 'maximumHeight',
                'x', 'y',
                'frameWidth', 'frameHeight',
                'lineWidth', 'lineHeight',
                'indent',
                'frame', 'autoScroll', 'autoScrollMargin',
                ))
        print(_pretty_format(data))

    if args.trigger:
        res = cl.send('CallVendorRequest', {
            'vendorName': 'ui-ws-automation',
            'requestType': 'menu-trigger',
            'requestData': {'path': args.path,},
        })
        print(res.response_data)

    if args.invoke_widget:
        data = {
                'path': args.path,
                'method': args.args[0],
        }
        for i, arg in enumerate(args.args):
            data[f'arg{i}'] = arg

        res = cl.send('CallVendorRequest', {
            'vendorName': 'ui-ws-automation',
            'requestType': 'widget-invoke',
            'requestData': data,
        })
        print(res.response_data)

if __name__ == '__main__':
    _main()
