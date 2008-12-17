import unittest
import threading

import support
import Tkinter

class BindTest(unittest.TestCase):

    def setUp(self):
        self.btn = Tkinter.Button()

    def tearDown(self):
        self.btn.destroy()


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
        funcid = self.btn.bind('<1>', lambda event: None)
        self.failUnless(funcid in self.btn._tclCommands)
        self.btn.unbind('<1>', funcid)
        self.failIf(funcid in self.btn._tclCommands)
        self.failIf(self.btn.bind())

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

    def test_unbind_all(self): pass

    def test_bind_class(self): pass

    def test_unbind_class(self): pass


def test_main():
    support.run(BindTest)

if __name__ == "__main__":
    test_main()
