struct RayPayload
{
    float2 bary;
    uint primitiveID;
    uint instanceID;
};
[shader("closesthit")]
void main(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.instanceID = InstanceIndex();
    payload.primitiveID = PrimitiveIndex();
    payload.bary = attr.barycentrics;
}