struct RayPayload
{
    float2 bary;
    uint primitiveID;
    uint instanceID;
};
[shader("miss")]
void main(inout RayPayload payload)
{
    payload.instanceID = 4294967295;
}