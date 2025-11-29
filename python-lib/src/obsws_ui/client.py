'''
Client to interact with the plugin through obs-websocket using 'obsws_python'
'''

from obsws_ui import common

class OBSWSUI:
    '''
    Context to comunicate with the plugin

    :param cl:  An instance of obsws_python.ReqClient.
    '''
    def __init__(self, cl):
        self.cl = cl

    def menu_list(self):
        '''Return a tree of menu actions

        The returned structure is a list of dict items.
        Each item has a key "menu", whose value is a list of children.
        '''
        res = self.cl.send('CallVendorRequest', common.request_menu_list())
        common.validate_response(res)
        return res.response_data['menu']

    def widget_list(self):
        '''Return a tree of widgets

        The returned structure is a list of dict items.
        Each item has a key "menu", whose value is a list of children.
        '''
        res = self.cl.send('CallVendorRequest', common.request_widget_list())
        common.validate_response(res)
        return res.response_data['children']

    def menu_trigger(self, path):
        '''Trigger a menu item specified by path

        :param path:  List of dict to find the menu item.
        '''
        res = self.cl.send('CallVendorRequest', common.request_menu_trigger(path))
        common.validate_response(res)

    def widget_invoke(self, path, method, *args):
        '''Invoke a method on a widget

        :param path:    List of dict to specify the widget.
        :param method:  Name of the method.
        :param args:    Arguments given to the method.
        '''

        res = self.cl.send('CallVendorRequest', common.request_widget_invoke(path, method, *args))
        common.validate_response(res)

    def widget_grab(self, path):
        '''Grab a widget

        :param path:  List of dict to specify the widget.
        '''
        res = self.cl.send('CallVendorRequest', common.request_widget_grab(path))
        return common.decode_widget_grab(res)
