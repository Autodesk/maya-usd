

# Workflows: How do we use AL_USDMaya in practice?

When working with AL_USDMaya as part of our Animation workflow, we have an artist-facing UI tool which we can launch inside maya, which allows us to choose a shot to work on. This is querying our Production/Asset Management DB for this information. This tool loads a USD shot file containing a list of USD "Models" e.g characters, props etc, Environments, Cameras and so on,  with all of the payloads disabled making it light and easy to work with - but exposing controls which artists need, such as the ability to switch variants, make things active/inactive, switch versions etc. This then records any modifications made to the shot usd file as a session layer - which is then loaded by AL_USDMaya, (with payloads expanded)

## Motion-editing Workflow

- Once we have told AL_USDMaya to load the scene (and in our UI swapped the USDStage it's looking at to view the one loaded in Maya) the Animator does their job. They can make changes to the in-memory USD scene (e.g swapping variants, loading additional characters etc) as they work. 
- After their work is complete, they can export e.g character or camera motion which are mostly stored in the maya domain. The export process and the subsequent publishing of assets is managed by our Animator UI. In future we would like to have this pass through the Schema Translator System in the form of a symmetrical export() method

## Environment-specific Workflow
At Animal Logic, the proxy Shape is mostly used to display environments, which are often large and don't scale well in maya (many of our Lego environments have billions of heavily instanced bricks). Mostly we're happy to just leave these things being displayed by Hydra in VP2, but sometimes we need some of them in Maya (but usually not all of them!)

We therefore support a hybrid (in the sense of neither pure USD nor pure Maya) approach by being able to easily create transforms in maya for objects being displayed in the USD proxy Shape. These transforms are "live" and are driven by the motion of the USD objects. We can also drive them the other way round (from maya to USD) so we can add constraints defined in maya to objects in USD.

We are currently adding full support for translating meshes and anything else which is selectable and displayed in the Hydra viewport (USDImageable?) to Maya 

Another requirement we have is to be able to override various attributes on the objects being displayed by the Proxy Shape - for example we may want to modify objects in an environment for our chosen shot (e.g moving furniture around). This can be achieved by choosing a named layer as an edit target (it can be a new, empty layer), and then performing operations such as transformation using Maya's Transform tools. These changes will be recorded in the current Edit Target and can then be serialised or changed further.
