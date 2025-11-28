#! /usr/bin/env python3
'''
Common methods
'''

import base64

_VENDOR_NAME = 'ui-ws-automation'

def validate_response(response):
    'Check the response from obs-websocket'
    data = response.response_data
    if 'error' in data:
        raise Exception(data['error'])

def request_menu_list():
    'Create a request to list menu actions'
    return {
            'vendorName': _VENDOR_NAME,
            'requestType': 'menu-list',
    }

def request_menu_trigger(path):
    'Create a request to trigger the menu'
    return {
            'vendorName': _VENDOR_NAME,
            'requestType': 'menu-trigger',
            'requestData': {'path': path,},
    }

def request_widget_list():
    'Create a request to list widgets'
    return {
            'vendorName': _VENDOR_NAME,
            'requestType': 'widget-list',
    }

def request_widget_invoke(path, method, *args):
    'Create a request to invoke a method on a widget'
    data = {
            'path': path,
            'method': method,
    }
    for i, arg in enumerate(args):
        data[f'arg{i+1}'] = arg

    return {
            'vendorName': _VENDOR_NAME,
            'requestType': 'widget-invoke',
            'requestData': data,
    }

def request_widget_grab(path):
    'Create a request to get an image of a widget'
    return {
            'vendorName': _VENDOR_NAME,
            'requestType': 'widget-grab',
            'requestData': {'path': path},
    }

def decode_widget_grab(response):
    'Decode the response of "widget-grab"'
    validate_response(response)
    return base64.b64decode(response.response_data['image'])
