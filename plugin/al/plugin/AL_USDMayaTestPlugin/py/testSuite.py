import unittest

import testProxyShape
import testTranslators
import testLayerManager

loader = unittest.TestLoader()
suite  = unittest.TestSuite()

# add tests to the test suite
suite.addTests(loader.loadTestsFromModule(testProxyShape))
suite.addTests(loader.loadTestsFromModule(testTranslators))
suite.addTests(loader.loadTestsFromModule(testLayerManager))

runner = unittest.TextTestRunner(verbosity=2)
result = runner.run(suite)

exit(not result.wasSuccessful())