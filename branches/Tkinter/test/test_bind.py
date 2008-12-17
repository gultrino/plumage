import unittest
import threading

import support
import Tkinter

class BindTest(unittest.TestCase):

    def setUp(self):
        self.btn = Tkinter.Button()

    def tearDown(self):
        self.btn.destroy()

    # XXX this may move to another module
    def test_bind_leak(self):
        # leak test: not removing a just created command because another call
        #            to Tcl failed
        orig_len = len(self.btn._tclCommands)
        try:
            self.btn.bind('<hi there>', lambda: None)
        except Tkinter.TclError:
            # command should be gone by now
            pass
        new_len = len(self.btn._tclCommands)

        self.failIf(new_len != orig_len, "(%d != %d) bind leaked" % (
            new_len, orig_len))

        # leak test: overwrite registered commnads
        def test(): pass
        def test2(): pass
        self.btn.bind('<1>', test)
        name = self.btn.bind('<1>')[0]
        self.btn.bind('<1>', test2)
        # the command associated to test should no longer exist now

        self.failIf(self.btn.tk.call("info", "commands", name))
        self.failUnlessEqual(len(self.btn.bind('<1>')), 1)
        self.failUnless(self.btn.bind('<1>')[0] != name)


    def test_bind(self):
        activated = []
        def test(event):
            activated.append(True)

        self.btn.bind('<1>', test)
        self.btn.pack()
        self.btn.wait_visibility()

        support.simulate_mouse_click(self.btn, 5, 5)
        self.failUnless(activated, "The callback wasn't invoked")

        self.failUnless(isinstance(self.btn.bind(), tuple))

    def test_unbind(self):
        self.btn.bind('<1>', lambda: None)
        self.failUnless(self.btn.bind())
        self.btn.unbind('<1>')
        self.failIf(self.btn.bind())

        self.btn.bind('<1>', lambda: None)
        self.btn.bind('<1>', lambda: None, add=True)
        curr_cmds = self.btn._tclCommands
        names = self.btn.bind('<1>')
        self.failUnlessEqual(len(names), 2)
        self.btn.unbind('<1>')
        self.failIf(curr_cmds)
        for name in names:
            self.failIf(self.btn.tk.call("info", "commands", name))

    def test_bind_all(self):
        btn = self.btn
        activated = [False, False]

        cb = lambda event: activated.pop()

        btn2 = Tkinter.Button()
        f1 = btn2.bind('<1>', cb)
        f2 = btn.bind('<1>', cb)

        btn2.pack()
        btn.pack()
        btn2.wait_visibility()
        btn.wait_visibility()

        btn.bind_all("<1>", cb)

        support.simulate_mouse_click(btn, 0, 0)
        self.failIf(activated)

    def test_unbind_all(self):
        self.btn.bind_all("<1>", lambda event: None)
        self.btn.bind_all("<1>", lambda event: None, add=True)
        self.failUnlessEqual(len(self.btn.bind_all("<1>")), 2)
        curr_cmds = self.btn._tclCommands
        names = self.btn.bind_all("<1>")
        self.btn.unbind_all("<1>")
        self.failIf(curr_cmds)
        for name in names:
            self.failIf(self.btn.tk.call("info", "commands", name))

    def test_bind_class(self): pass

    def test_unbind_class(self): pass


def test_main():
    support.run(BindTest)

if __name__ == "__main__":
    test_main()
